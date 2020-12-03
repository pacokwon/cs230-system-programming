#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

void handle_connection(int fd);
void read_request_headers(rio_t *rp);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
int parse_uri(char *uri, char *host, char *port, char *path);
void handle_request(rio_t *conn_rio, rio_t *client_rio, int clientfd, int connfd, char *method, char *host, char *path);
int is_extra_header(char *str, int length);

int main(int argc, char **argv) {
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        return 1;
    }

    /* open a listen file descriptor on <port> */

    listenfd = Open_listenfd(argv[1]);
    while (1) {
        clientlen = sizeof(clientaddr);

        /* connfd stores the connected file descriptor */
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);

        handle_connection(connfd);
        Close(connfd);
    }

    return 0;
}

void handle_connection(int fd) {
    rio_t rio;
    char buf[MAXLINE];

    Rio_readinitb(&rio, fd);

    /* this will read header information */
    /* header information is in the <method> <uri> <version> syntax */
    Rio_readlineb(&rio, buf, MAXLINE);
    printf("Request headers:\n");
    printf("%s", buf);

    char method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    sscanf(buf, "%s %s %s", method, uri, version);

    /* port is at most 16 bits */
    char host[MAXLINE], port[10], path[MAXLINE];
    parse_uri(uri, host, port, path);
    printf("host: %s\tport: %s\tpath: %s\n", host, port, path);

    /* at this point, `host`, `port` and `path` are filled in */
    /* open new file descriptor */

    int clientfd = Open_clientfd(host, port);
    if (clientfd < 0)
        return;

    rio_t client_rio;
    Rio_readinitb(&client_rio, clientfd);

    handle_request(&rio, &client_rio, clientfd, fd, method, host, path);
    Close(clientfd);
}

void read_request_headers(rio_t *rp) {
    char buf[MAXLINE];

    Rio_readlineb(rp, buf, MAXLINE);
    while (strcmp(buf, "\r\n")) {
        Rio_readlineb(rp, buf, MAXLINE);
        printf("%s", buf);
    }

    return;
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) {
    char buf[MAXLINE], body[MAXBUF];

    sprintf(body, "<html><title>Proxy Error</title>");
    sprintf(body, "%s<body style='background: #ffffff'>\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>Proxy Server</em>\r\n", body);

    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));

    return;
}

/*
 * parse_uri - accept a uri string, parse the uri and initialize the
 *   host, path pointers.
 */
int parse_uri(char *uri, char *host, char *port, char *path) {
    /* uri starts from path */
    if (uri[0] == '/') {
        strcpy(path, uri);
        return 0;
    }

    /* must start with http */
    if (strncasecmp(uri, "http://", 7))
        return 1;
    uri = uri + 7;

    /* find port colon */
    int port_num = 80;
    char *port_ptr = strchr(uri, ':');
    if (port_ptr) { // reassign port if exists
        port_num = atoi(port_ptr + 1);

        // copy until just before the colon
        strncpy(host, uri, port_ptr - uri);
    }

    sprintf(port, "%d", port_num);

    char *ptr = strchr(uri, '/');
    /* no slash at the end */
    if (!ptr) {
        /* if port was specified, `host` would already have been filled in */
        if (!port_ptr)
            strcpy(host, uri);

        strcpy(path, "/");
        return 0;
    }

    /* from here, we know that `ptr` points to a slash(/) */

    /* the block of memory b/t uri ~ ptr is the host */
    /* if port was specified, `host` would already have been filled in */
    if (!port_ptr) {
        strncpy(host, uri, ptr - uri);
        host[ptr - uri] = '\0';
    }

    /* the block of memory starting from uri is the path */
    strcpy(path, ptr);

    return 0;
}


/*
 * handle_request - pass client headers to host and send a request. then, pass the response back to the client
 */
void handle_request(rio_t *conn_io, rio_t *client_rio, int clientfd, int connfd, char *method, char *host, char *path) {
    char buf[MAXLINE];
    /* request header information is in the <method> <uri> <version> syntax */
    sprintf(buf, "%s %s %s\r\n", method, path, "HTTP/1.0");
    Rio_writen(clientfd, buf, strlen(buf));

    /* start writing header */
    sprintf(buf, "Host: %s\r\n", host);
    Rio_writen(clientfd, buf, strlen(buf));

    sprintf(buf, "User-Agent: %s\r\n", user_agent_hdr);
    Rio_writen(clientfd, buf, strlen(buf));

    sprintf(buf, "Connection: close\r\n");
    Rio_writen(clientfd, buf, strlen(buf));

    sprintf(buf, "Proxy-Connection: close\r\n\r\n");
    Rio_writen(clientfd, buf, strlen(buf));

    Rio_readlineb(conn_io, buf, MAXLINE);
    while (strcmp(buf, "\r\n")) {
        /* `buf` will have a format of header: value, */
        /* and `colon` will point to the colon */
        char *colon = strchr(buf, ':');

        /* if `buf` does not contain a colon, it is an invalid header */
        /* also, if `buf` is not an extra header, skip it */
        if (colon && is_extra_header(buf, colon - buf))
            Rio_writen(clientfd, buf, strlen(buf));

        Rio_readlineb(conn_io, buf, MAXLINE);
    }

    ssize_t bytes_read;
    while ((bytes_read = Rio_readlineb(client_rio, buf, MAXLINE)) > 0)
        Rio_writen(connfd, buf, strlen(buf));
}

/*
 * is_extra_header - returns if a given header is NOT one of the default headers
 *   in this context, a default header is one of the following:
 *   Host, User-Agent, Connection, and Proxy-Connection
 */
int is_extra_header(char *str, int length) {
    char *default_headers[] = { "Host", "User-Agent", "Connection", "Proxy-Connection" };

    for (int i = 0; i < 4; i++) {
        if (!strncasecmp(default_headers[i], str, length))
            return 0;
    }

    return 1;
}
