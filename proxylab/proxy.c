#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

void *thread(void* vargp);
void handle_connection(int connfd);
int parse_uri(char *uri, char *host, char *port, char *path);
void handle_request(rio_t *conn_rio, rio_t *client_rio, int clientfd, int connfd, char *method, char *host, char *path);
int is_extra_header(char *str, int length);

int main(int argc, char **argv) {
    /* ignore SIGPIPE signals */
    signal(SIGPIPE, SIG_IGN);

    int listenfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    pthread_t tid;
    struct sockaddr_storage clientaddr;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        return 1;
    }

    /* open a listen file descriptor on <port> */

    listenfd = Open_listenfd(argv[1]);
    while (1) {
        clientlen = sizeof(clientaddr);

        int *connfdp = malloc(sizeof(int));
        /* connfdp stores a pointer to the connected file descriptor */
        *connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);

        Pthread_create(&tid, NULL, thread, connfdp);
    }

    Close(listenfd);
    return 0;
}

/*
 * thread - spawn thread to handle connected file descriptor requests
 *   a pointer to an integer(connected file descriptor) is passed as an argument.
 *   this function calls `handle_connection` with the given argument.
 *   this thread runs in detached mode, so that it will clean itself up.
 *   the passed pointer is dynamically allocated, so we free it with the fd before returning.
 */
void *thread(void* vargp) {
    int connfd = *(int *)(vargp);
    Pthread_detach(Pthread_self());
    free(vargp);
    handle_connection(connfd);
    Close(connfd);
    return NULL;
}

/*
 * handle_connection - receive requests from the connected file descriptor,
 *   then pass it to the host. After that, receive responses from the host and
 *   pass it to the connected file descriptor.
 */
void handle_connection(int connfd) {
    rio_t rio;
    char buf[MAXLINE];

    Rio_readinitb(&rio, connfd);

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

    handle_request(&rio, &client_rio, clientfd, connfd, method, host, path);
    Close(clientfd);
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

    /* read request headers from client */
    Rio_readlineb(conn_io, buf, MAXLINE);
    while (strcmp(buf, "\r\n")) {
        /* `buf` will have a format of header: value, */
        /* and `colon` will point to the colon */
        char *colon = strchr(buf, ':');

        /* if `buf` does not contain a colon, it is an invalid header */
        /* also, if `buf` is not an extra header, skip it */
        /* forward headers to server */
        if (colon && is_extra_header(buf, colon - buf))
            Rio_writen(clientfd, buf, strlen(buf));

        Rio_readlineb(conn_io, buf, MAXLINE);
    }

    char header_buf[MAXLINE], value_buf[MAXLINE];
    int content_length = -1;

    /* read response headers from server */
    Rio_readlineb(client_rio, buf, MAXLINE);
    while (strcmp(buf, "\r\n")) {
        sscanf(buf, "%[^:]: %s", header_buf, value_buf);
        if (!strcasecmp(header_buf, "Content-Length"))
            content_length = atoi(value_buf);

        Rio_writen(connfd, buf, strlen(buf));
        Rio_readlineb(client_rio, buf, MAXLINE);
    }
    Rio_writen(connfd, buf, strlen(buf));

    /* there is no content. return. */
    if (content_length <= 0)
        return;

    /* current buffer is big enough */
    if (content_length <= MAXLINE) {
        Rio_readnb(client_rio, buf, content_length);
        Rio_writen(connfd, buf, content_length);
    }

    /* current buffer is not big enough. allocate new buffer on heap */
    else {
        void *alloc_buf = malloc(content_length);
        Rio_readnb(client_rio, alloc_buf, content_length);
        Rio_writen(connfd, alloc_buf, content_length);
        free(alloc_buf);
    }
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
