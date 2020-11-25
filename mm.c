/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/* single word (8) or double word (16) alignment */
#define ALIGNMENT 16

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE       8           // word size (also header & footer size)
#define DSIZE       16          // double word size
#define CHUNKSIZE   (1 << 12)   // bytes

#define MAX(x, y)   ((x) > (y) ? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)   ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)          (*(unsigned int *)(p))
#define PUT(p, val)     (*(unsigned int *)(p) = (val))

#define GETP(p)         (*(char **)(p))
#define PUTP(p, val)    (*(char **)(p) = (val))

#define GET_SIZE(p)     (GET(p) & ~0x7)
#define GET_ALLOC(p)    (GET(p) & 0x1)

/* bp starts at payload */
#define HDRP(bp)        ((char *)(bp) - WSIZE)
/* since bp starts at payload, subtract double word size */
#define FTRP(bp)        ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* get address of next free block. effective equivalent of GETP */
#define GET_NEXTP(bp)   (*(char **)(bp))

/* get address of previous free block */
#define GET_PREVP(bp)   (*((char **)((bp) + WSIZE)))  // get pp from bp

#define ADJ_PREVP(bp)   ((char *)(bp) + WSIZE)  // get pp from bp
#define ADJ_NEXTP(pp)   ((char *)(pp) - WSIZE)  // get bp from pp

/* get next block */
#define NEXT_BLKP(bp)   ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
/* get prev block */
#define PREV_BLKP(bp)   ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

extern int verbose;

char *heap_listp;
char *free_head;     // pointer to bp of first node

static void *coalesce(void *bp);
static void *extend_heap(size_t words);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);
static void insert_first(void* bp);
static void remove_node(void *bp);
void mm_check();

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
    if ((heap_listp = mem_sbrk(6 * WSIZE)) == (void*)-1)
        return -1;

    PUT(heap_listp, 0);
    PUT(heap_listp + 1 * WSIZE, PACK(4 * WSIZE, 1));
    PUTP(heap_listp + 2 * WSIZE, NULL); // dummy node next
    PUTP(heap_listp + 3 * WSIZE, NULL); // dummy node prev
    PUT(heap_listp + 4 * WSIZE, PACK(4 * WSIZE, 1));
    PUT(heap_listp + 5 * WSIZE, PACK(0, 1));

    heap_listp += (2 * WSIZE);
    free_head = heap_listp;

    char *bp;
    /* Allocate CHUNKSIZE bytes ahead of time */
    if ((bp = extend_heap(CHUNKSIZE / WSIZE)) == NULL)
        return -1;

    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
    if (verbose)
        printf("Entering mm_malloc\n");
    size_t newsize, extendsize;
    char *bp;

    if (size == 0)
        return NULL;

    if (size <= DSIZE)
        newsize = 2 * DSIZE;    // header, footer, and minimum payload
    else
        newsize = 2 * WSIZE + ALIGN(size);

    if ((bp = find_fit(newsize)) != NULL) {
        place(bp, newsize);
        if (verbose > 1)
            mm_check();
        return bp;
    }

    extendsize = MAX(newsize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;

    place(bp, newsize);
    if (verbose > 1)
        mm_check();
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp) {
    if (verbose)
        printf("Entering mm_free\n");
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);

    if (verbose > 1)
        mm_check();
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) {
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
        copySize = size;

    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}

static void *coalesce(void *bp) {
    if (verbose)
        printf("Entering coalesce\n");

    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (verbose)
        printf("prev: %lu\tnext: %lu\n", prev_alloc, next_alloc);

    if (prev_alloc && next_alloc) {
        // do nothing
    } else if (prev_alloc && !next_alloc) {
        // next one is free
        remove_node(NEXT_BLKP(bp));

        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    } else if (!prev_alloc && next_alloc) {
        // prev one is free
        remove_node(PREV_BLKP(bp));

        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        bp = PREV_BLKP(bp);

        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    } else {
        // both sides are free
        remove_node(PREV_BLKP(bp));
        remove_node(NEXT_BLKP(bp));

        size += GET_SIZE(HDRP(NEXT_BLKP(bp))) + GET_SIZE(HDRP(PREV_BLKP(bp)));
        bp = PREV_BLKP(bp);

        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
    }

    insert_first(bp);
    return bp;
}

static void *extend_heap(size_t words) {
    if (verbose)
        printf("\nEntering extend_heap\n");

    char *bp;
    size_t size;

    // we need an even number of words
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    if (verbose)
        printf("Extended heap with %lu bytes\n", size);

    /* initialize header and footer */
    PUT(HDRP(bp), PACK(size, 0));   // this will overwrite the old epilogue block
    PUT(FTRP(bp), PACK(size, 0));

    /* new epilogue block */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    return coalesce(bp);
}


static void *find_fit(size_t asize) {
    void *bp = free_head;

    while (bp != NULL) {
        /* bp is not allocated and free space is enough */
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp))))
            return bp;

        bp = GET_NEXTP(bp);
    }

    return NULL;
}

static void place(void *bp, size_t asize) {
    /* prohibit prologue block access */
    if (bp == free_head)
        return;

    char* prev_pp = GET_PREVP(bp);
    char* prev_bp = ADJ_NEXTP(prev_pp);

    char* next_bp = GET_NEXTP(bp); // might be null

    size_t csize = GET_SIZE(HDRP(bp));
    if ((csize - asize) >= (2 * DSIZE)) {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));

        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));

        /* modify four connections */
        PUTP(prev_bp, bp);
        PUTP(ADJ_PREVP(bp), prev_pp);
        PUTP(bp, next_bp);
        if (next_bp != NULL)
            PUTP(ADJ_PREVP(next_bp), ADJ_PREVP(bp));
    } else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));

        /* modify two connections */
        PUTP(prev_bp, next_bp);
        if (next_bp != NULL)
            PUTP(ADJ_PREVP(next_bp), prev_pp);
    }
}

static void insert_first(void* bp) {
    char* cur_np = bp;
    char* cur_pp = ADJ_PREVP(bp);

    PUTP(cur_np, GET_NEXTP(free_head));
    if (GET_NEXTP(free_head) != NULL)
        PUTP(ADJ_PREVP(GET_NEXTP(free_head)), cur_pp);
    PUTP(free_head, cur_np);
    PUTP(cur_pp, ADJ_PREVP(free_head));
}

static void remove_node(void *bp) {
    if (verbose)
        printf("Entering remove_node\n");

    /* prohibit prologue block access */
    if (bp == free_head)
        return;

    char* prev_pp = GET_PREVP(bp);  // guaranteed to be not null
    char* prev_bp = ADJ_NEXTP(prev_pp);
    char* next_bp = GET_NEXTP(bp);  // might be null

    /* modify two connections */
    PUTP(prev_bp, next_bp);
    /* if bp is not last node */
    if (next_bp != NULL)
        PUTP(ADJ_PREVP(next_bp), prev_pp);
}

void mm_check() {
    printf("------- Iterating blocks ------\n");
    char* iter = NEXT_BLKP(heap_listp);
    size_t count = 1;
    printf("size: %u\n", GET_SIZE(HDRP(iter)));
    while (GET_SIZE(HDRP(iter)) != 0) {
        printf("Block number %03lu\n", count);
        printf("\tSize:%8d\tAllocated:%4d\tAddress: %p\n", GET_SIZE(HDRP(iter)), GET_ALLOC(FTRP(iter)), iter);
        iter = NEXT_BLKP(iter);
        count += 1;
    }
    printf("-------------------------------\n");

    printf("----- Iterating free list -----\n");
    char* ptr = GET_NEXTP(free_head);

    printf("ptr: %p\n", ptr);
    count = 1;
    while (ptr != NULL) {
        printf("Block number %03lu\n", count);
        printf("\tSize:%8d\tAllocated:%4d\n", GET_SIZE(HDRP(ptr)), GET_ALLOC(HDRP(ptr)));
        ptr = GET_NEXTP(ptr);
        count += 1;
    }
    printf("-------------------------------\n");
}
