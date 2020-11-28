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
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0xF)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE       8           // word size (also header & footer size)
#define DSIZE       16          // double word size
#define CHUNKSIZE   (1 << 6)   // bytes

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

/* number of `size classes` */
#define SIZE_CLASS_SIZE 8

extern int verbose;
extern int heap_check_flag;

char *heap_listp;

/*
Segregated Free List:
Since minimum block size is 32, size classes will look like:
    {32 ~ 63}, {64 ~ 127}, ..., {2048 ~ 4095}, {4096 ~ inf}
This is 8 entries, so we'll keep an array of pointers of size 8.
*/

/* array of char pointers */
static char **heads;

char *epilogue;     // pointer to bp of first node

static void *coalesce(void *bp);
static void *extend_heap(size_t words);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);
static void insert_first(void* bp);
static void remove_node(void *bp);

static int heap_check();
static int heap_check_free();
static int heap_check_cross_free();
static int heap_check_overlap();

static size_t get_size_class();
void mm_check();

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
    heads = (char**) mem_sbrk(SIZE_CLASS_SIZE * WSIZE);
    for (size_t index = 0; index < SIZE_CLASS_SIZE; index++)
        heads[index] = NULL;

    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void*)-1)
        return -1;

    PUT(heap_listp, 0);
    PUT(heap_listp + 1 * WSIZE, PACK(2 * WSIZE, 1));
    PUT(heap_listp + 2 * WSIZE, PACK(2 * WSIZE, 1));
    PUT(heap_listp + 3 * WSIZE, PACK(0, 1));

    epilogue = heap_listp + 3 * WSIZE;
    heap_listp += (2 * WSIZE);

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
        printf("Entering mm_malloc()\n");

    size_t newsize, extendsize;
    char *bp;

    if (size == 0)
        return NULL;

    if (size <= DSIZE)
        newsize = DSIZE + DSIZE;            // header + footer -> DSIZE. minimum payload -> DSIZE
    else
        newsize = DSIZE + ALIGN(size);      // header + footer -> DSIZE. aligned payload -> N * DSIZE

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

    if (heap_check_flag)
        if (heap_check() && verbose)
            printf("Heap compromised!\n");

    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp) {
    if (verbose)
        printf("Entering mm_free()\n");

    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);

    if (verbose > 1)
        mm_check();

    if (heap_check_flag)
        if (heap_check() && verbose)
            printf("Heap compromised!\n");
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) {
    if (verbose)
        printf("Entering mm_realloc()\n");

    void *oldptr = ptr;
    void *newptr;
    size_t copy_size;

    if (size == 0) {
        if (verbose)
            printf("realloc requested size is 0.\n");

        mm_free(oldptr);
        return NULL;
    }

    if (oldptr == NULL) {
        if (verbose)
            printf("old pointer value is NULL.\n");

        return mm_malloc(size);
    }

    size_t original_size = GET_SIZE(HDRP((char*)oldptr));
    size_t aligned_size = ALIGN(size) + DSIZE;

    if (aligned_size == original_size) {
        if (verbose)
            printf("Same size as original block is requested.\n");

        return oldptr;
    }

    /* from here, oldptr is valid, size is not 0, */
    /* and original_size > aligned_size or original_size < aligned_size */

    /* current allocated block is large enough to hold new size */
    if (aligned_size < original_size) {
        if (verbose)
            printf("Originally allocated size is larger than requested. OG size: %lu\tReq size: %lu\n", original_size, aligned_size);

        /* use the entire allocated block */
        if (original_size <= copy_size + 2 * DSIZE)
            return oldptr;

        if (verbose)
            printf("Splitting original block.\n");
        /* split current allocated block */
        PUT(HDRP(oldptr), PACK(aligned_size, 1));
        PUT(FTRP(oldptr), PACK(aligned_size, 1));
        newptr = oldptr;

        /* now, mark the splitted block as free */
        oldptr = NEXT_BLKP(oldptr);
        PUT(HDRP(oldptr), PACK(original_size - aligned_size, 0));
        PUT(FTRP(oldptr), PACK(original_size - aligned_size, 0));

        /* coalesce and insert new free block into appropriate linked list */
        coalesce(oldptr);

        return newptr;
    }

    /* current allocated block is not large enough, but search for adjacent free blocks */
    size_t additional_required_size = aligned_size - original_size;

    void* next_bp = NEXT_BLKP(oldptr);
    /* search for next block */
    if (next_bp != NULL && !GET_ALLOC(HDRP(next_bp))) {
        size_t next_size = GET_SIZE(HDRP(next_bp));

        if (verbose)
            printf("Inspecting next block. og size: %lu\tnext size: %lu\treq size: %lu\taddreqsize: %lu\n", original_size, next_size, aligned_size, additional_required_size);

        /* next block's space is more than enough, split free block */
        if (next_size >= additional_required_size + 2 * DSIZE) {
            if (verbose)
                printf("More than enough space\n");

            /* remove free block originally at next_bp. we'll replace it with a new one */
            remove_node(next_bp);

            PUT(HDRP(oldptr), PACK(aligned_size, 1));
            PUT(FTRP(oldptr), PACK(aligned_size, 1));
            newptr = oldptr;

            next_bp = next_bp + additional_required_size;
            PUT(HDRP(next_bp), PACK(next_size - additional_required_size, 0));
            PUT(FTRP(next_bp), PACK(next_size - additional_required_size, 0));
            /* coalesce and insert new free block into appropriate linked list */
            coalesce(next_bp);

            return newptr;
        }

        /* if there's just enough space, take the entire next block */
        else if (next_size >= additional_required_size) {
            /* next_bp no longer contains a free block */
            remove_node(next_bp);

            PUT(HDRP(oldptr), PACK(original_size + next_size, 1));
            PUT(FTRP(oldptr), PACK(original_size + next_size, 1));
            /* no need to coalesce and insert, as there are no free blocks */
            return oldptr;
        }

        /* not enough space in the next block */
        if (verbose)
            printf("Not enough space\n");
    }

    void* prev_bp = PREV_BLKP(oldptr);
    if (prev_bp != NULL && !GET_ALLOC(HDRP(prev_bp))) {
        size_t prev_size = GET_SIZE(HDRP(prev_bp));

        if (verbose)
            printf("Inspecting prev block. og size: %lu\tprev size: %lu\treq size: %lu\taddreqsize: %lu\n", original_size, prev_size, aligned_size, additional_required_size);

        /* prev block's space is more than enough, split free block */
        if (prev_size >= 2 * DSIZE + additional_required_size) {
            /* remove node at prev_bp. its signature will be different.          */
            /* this must be done before PUT potentially overwrites any link info */
            remove_node(prev_bp);

            newptr = oldptr - additional_required_size;
            PUT(HDRP(newptr), PACK(aligned_size, 1));
            PUT(FTRP(newptr), PACK(aligned_size, 1));
            memmove(newptr, oldptr, size);

            PUT(HDRP(prev_bp), PACK(prev_size - additional_required_size, 0));
            PUT(FTRP(prev_bp), PACK(prev_size - additional_required_size, 0));

            coalesce(prev_bp);

            return newptr;
        }

        /* if there's just enough space, take the entire prev block */
        else if (prev_size >= additional_required_size) {
            /* prev_bp no longer contains a free block */
            remove_node(prev_bp);

            PUT(HDRP(prev_bp), PACK(prev_size + original_size, 1));
            PUT(FTRP(prev_bp), PACK(prev_size + original_size, 1));
            memmove(prev_bp, oldptr, size);

            return prev_bp;
        }

        /* not enough space in the next block */
    }
    if (verbose)
        printf("Allocating new memory.\n");

    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;

    copy_size = GET_SIZE(HDRP(ptr)) - DSIZE;

    /* realloc request size is smaller than originally allocated */
    if (size < copy_size)
        copy_size = size;

    memcpy(newptr, oldptr, copy_size);
    mm_free(oldptr);

    if (heap_check_flag)
        if (heap_check() && verbose)
            printf("Heap compromised!\n");

    return newptr;
}

static void *coalesce(void *bp) {
    if (verbose)
        printf("Entering coalesce()\n");

    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (verbose)
        printf("prev: %lu\tnext: %lu\tsize: %lu\taddr: %p\n", prev_alloc, next_alloc, size, bp);

    if (prev_alloc && next_alloc) {
        // do nothing
    } else if (prev_alloc && !next_alloc) {
        if (verbose)
            printf("Next block size: %u\taddr: %p\n", GET_SIZE(HDRP(NEXT_BLKP(bp))), NEXT_BLKP(bp));

        // next one is free
        remove_node(NEXT_BLKP(bp));

        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    } else if (!prev_alloc && next_alloc) {
        if (verbose)
            printf("Prev block size: %u\taddr: %p\n", GET_SIZE(HDRP(PREV_BLKP(bp))), PREV_BLKP(bp));

        // prev one is free
        remove_node(PREV_BLKP(bp));

        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        bp = PREV_BLKP(bp);

        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    } else {
        if (verbose) {
            printf("Prev block size: %u\taddr: %p\n", GET_SIZE(HDRP(PREV_BLKP(bp))), PREV_BLKP(bp));
            printf("Next block size: %u\taddr: %p\n", GET_SIZE(HDRP(NEXT_BLKP(bp))), NEXT_BLKP(bp));
        }

        // both sides are free
        remove_node(PREV_BLKP(bp));
        remove_node(NEXT_BLKP(bp));

        size += GET_SIZE(HDRP(NEXT_BLKP(bp))) + GET_SIZE(HDRP(PREV_BLKP(bp)));

        bp = PREV_BLKP(bp);

        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    insert_first(bp);

    if (verbose)
        printf("After coalesce - size: %u\taddr: %p\n", GET_SIZE(HDRP(bp)), bp);

    return bp;
}

static void *extend_heap(size_t words) {
    if (verbose)
        printf("Entering extend_heap()\n");

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
    epilogue = NEXT_BLKP(bp);

    return coalesce(bp);
}


static void *find_fit(size_t asize) {
    if (verbose)
        printf("Entering find_fit()\n");

    /* lower bound of size class. starts from 32 (inclusive) */
    size_t lower_bound = 2 * DSIZE;

    size_t size_class_index = 0;
    while (
        size_class_index < SIZE_CLASS_SIZE &&
        !(lower_bound <= asize && asize < (lower_bound * 2))
    ) {
        size_class_index += 1;
        lower_bound *= 2;
    }

    if (size_class_index == SIZE_CLASS_SIZE)
        size_class_index -= 1;

    /* now, size_class_index stores the index to the size class in `heads`  */
    /*  and lower_bound stores the lower bound of the size class(inclusive) */

    /* here, we inspect the linked list at `size_class_index` in `heads` */
    void *bp = heads[size_class_index];

    while (bp != NULL) {
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
            if (verbose)
                printf("Fit found at appropriate size class %lu.\n", size_class_index);

            return bp;
        }

        bp = GET_NEXTP(bp);
    }

    /* no fitting size at the size class. this can either be because the */
    /* linked list is empty, or there really is no fitting free block    */

    /* here, we inspect the larger size classes for finding a fitting free block */
    for (size_t index = size_class_index + 1; index < SIZE_CLASS_SIZE; index++) {
        bp = heads[index];

        while (bp != NULL) {
            if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
                if (verbose)
                    printf("Fit found at size class %lu.\n", index);

                return bp;
            }

            bp = GET_NEXTP(bp);
        }

        /* no fitting size found. move onto next size class */
    }

    /* at this point, there is no fitting block in any of the size classes */
    /* above the appropriate size class. so there is no fitting block */

    return NULL;
}

/*
 * place: attempt to allocate memory of size `asize`
 *      in the free block at address `bp`
 */
static void place(void *bp, size_t asize) {
    if (verbose)
        printf("Entering place()\n");

    /* prohibit prologue block access */
    char* prev_pp = GET_PREVP(bp); // might be null
    char* next_bp = GET_NEXTP(bp); // might be null

    size_t csize = GET_SIZE(HDRP(bp));
    size_t size_difference = csize - asize;
    if (size_difference >= (2 * DSIZE)) {
        /* first, remove the whole node at bp */
        remove_node(bp);
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));

        /* now, we have a new free block of size `size_difference` @ address `bp` */
        /* we must find the appropriate size class and insert this free block into that linked list */
        /* TODO!! */
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(size_difference, 0));
        PUT(FTRP(bp), PACK(size_difference, 0));
        insert_first(bp);
    } else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));

        /* remove the whole node at bp, as this whole block is allocated */
        remove_node(bp);
    }
}

/*
 * insert_first - insert node at bp as first node of linked list of appropriate size class
 *      NOTE: the block sizes must be specified at the header and footer,
 *      as this function makes use of that to guess the size class
 */
static void insert_first(void* bp) {
    if (verbose)
        printf("Entering insert_first()\n");

    char* cur_np = bp;
    char* cur_pp = ADJ_PREVP(bp);
    size_t size_class_index = get_size_class(GET_SIZE(HDRP(bp)));

    if (heads[size_class_index] == NULL) { // no element in linked list
        /* modify only two links */
        heads[size_class_index] = cur_np; // point to bp as first element
        PUTP(cur_np, NULL);
        PUTP(cur_pp, NULL);
    } else { // at least a single element exists
        /* modify four links */
        PUTP(cur_np, heads[size_class_index]);              // current next -> first element's next
        PUTP(ADJ_PREVP(heads[size_class_index]), cur_pp);   // first element's prev -> current prev
        heads[size_class_index] = cur_np;                   // update head to current element
        PUTP(cur_pp, NULL);                                 // put NULL at current prev
    }
}

/*
 * remove_node - safely remove node at address `bp` from linked list
 *      "safely removing" means linking previous and next nodes(if any)
 *      so that the linked list would remain valid
 *      NOTE: the block sizes must be specified at the header and footer,
 *      as this function makes use of that to guess the size class
 */
static void remove_node(void *bp) {
    if (verbose)
        printf("Entering remove_node()\n");

    char* prev_pp = GET_PREVP(bp);
    char* next_bp = GET_NEXTP(bp);
    size_t size_class_index = get_size_class(GET_SIZE(HDRP(bp)));

    /* case 0: bp is only element */
    if (prev_pp == NULL && next_bp == NULL) {
        heads[size_class_index] = NULL;
    }
    /* case 1: bp is first element */
    else if (prev_pp == NULL) {
        PUTP(ADJ_PREVP(next_bp), NULL); // set next's pp to NULL
        heads[size_class_index] = next_bp;
    }
    /* case 2: bp is last element */
    else if (next_bp == NULL) {
        PUTP(ADJ_NEXTP(prev_pp), NULL); // set prev's bp to NULL
    }
    /* case 3: bp is in between */
    else {
        PUTP(ADJ_NEXTP(prev_pp), next_bp); // link prev's bp to next's bp
        PUTP(ADJ_PREVP(next_bp), prev_pp); // link next's pp to prev's pp
    }
}

void mm_check() {
    printf("------- Iterating blocks ------\n");
    char* iter = NEXT_BLKP(heap_listp);
    size_t count = 1;
    while (GET_SIZE(HDRP(iter)) != 0) {
        printf("Block number %03lu\n", count);
        printf("\tSize:%8d\tAllocated:%4d\tAddress: %p\n", GET_SIZE(HDRP(iter)), GET_ALLOC(FTRP(iter)), iter);
        iter = NEXT_BLKP(iter);
        count += 1;
    }
    printf("Epilogue\n\tSize:%8d\tAllocated:%4d\tAddress: %p\n", GET_SIZE(HDRP(iter)), GET_ALLOC(HDRP(iter)), iter);
    printf("-------------------------------\n");

    printf("----- Iterating free list -----\n");

    for (size_t i = 0; i < SIZE_CLASS_SIZE; i++) {
        char* ptr = heads[i];

        printf("ptr: %p\n", ptr);
        count = 1;
        while (ptr != NULL) {
            printf("Block number %03lu\n", count);
            printf("\tClass:%2lu\tSize:%8d\tAllocated:%4d\tNext: %p\n", i, GET_SIZE(HDRP(ptr)), GET_ALLOC(HDRP(ptr)), GET_NEXTP(ptr));
            ptr = GET_NEXTP(ptr);
            count += 1;
        }
    }

    printf("-------------------------------\n");
}

static int heap_check() {
    if (verbose)
        printf("Entering heap_check()\n");

    if (heap_check_free()) {
        if (verbose)
            printf("-- Free block test failed!\n");

        verbose = 0;
        return 1;
    }

    if (heap_check_cross_free()) {
        if (verbose)
            printf("-- Free block cross check failed!\n");

        verbose = 0;
        return 1;
    }

    if (heap_check_overlap()) {
        if (verbose)
            printf("-- Overlap check failed!\n");

        verbose = 0;
        return 1;
    }

    return 0;
}

/* - Is every block in the free list marked as free? */
/* - Are there any contiguous free blocks that somehow escaped coalescing? */
/* - Do the pointers in the free list point to valid free blocks? */
static int heap_check_free() {
    for (size_t i = 0; i < SIZE_CLASS_SIZE; i++) {
        char* iter = heads[i];

        while (iter != NULL) {
            if (GET_ALLOC(HDRP(iter)) || GET_ALLOC(FTRP(iter))) {
                if (verbose)
                    printf("\tAllocated bit is set in free block!\n");
                return 1;
            }

            char* next = NEXT_BLKP(iter);
            if (next != NULL && !GET_ALLOC(HDRP(next))) {
                if (verbose)
                    printf("\tNext adjacent block is also free!\n");
                return 1;
            }

            char* prev = PREV_BLKP(iter);
            if (prev != NULL && !GET_ALLOC(HDRP(prev))) {
                if (verbose)
                    printf("\tPrev adjacent block is also free!\n");
                return 1;
            }

            iter = GET_NEXTP(iter);
        }
    }

    return 0;
}

/* - Is every free block actually in the free list? */
static int heap_check_cross_free() {
    char *block_iter = NEXT_BLKP(heap_listp);

    while (block_iter < epilogue) {
        if (GET_ALLOC(HDRP(block_iter))) {
            block_iter = NEXT_BLKP(block_iter);
            continue;
        }

        /* check for block_iter from segregated list */
        int exists = 0;
        for (size_t i = 0; i < SIZE_CLASS_SIZE; i++) {
            /* check size class at index `i` */

            // block_iter points to a free block
            char *free_iter = heads[i];

            // check if block_iter overlaps with free_iter
            while (free_iter != NULL) {
                if (block_iter == free_iter) {
                    exists = 1;
                    break;
                }

                free_iter = GET_NEXTP(free_iter);
            }

            if (exists)
                break;
        }

        if (!exists) {
            if (verbose)
                printf("\tFree block does not exist in linked list!\n");
            return 1;
        }

        block_iter = NEXT_BLKP(block_iter);
    }

    return 0;
}

/* - Do any allocated blocks overlap? */
static int heap_check_overlap() {
    char *iter = NEXT_BLKP(heap_listp);

    while (GET_SIZE(HDRP(iter)) > 0) {
        if (!GET_ALLOC(HDRP(iter))) {
            iter = NEXT_BLKP(iter);
            continue;
        }

        size_t size = GET_SIZE(HDRP(iter));
        char* next = NEXT_BLKP(iter);
        if (iter + size != next)
            return 1;

        iter = next;
    }

    return 0;
}

/* get_size_class: get size class of given block size
 *      this function returns the index of `heads`
 *      that the size class would fit in
 */
static size_t get_size_class(size_t asize) {
    size_t lower_bound = 32;
    size_t index = 0;

    while (
        index < SIZE_CLASS_SIZE &&
        !(lower_bound <= asize && asize < (lower_bound * 2))
    ) {
        lower_bound *= 2;
        index += 1;
    }

    if (index == SIZE_CLASS_SIZE)
        index -= 1;

    return index;
}
