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
 *
 *
 */
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1 << 12)

static void* heap_listp;

static int max(int x, int y) {
    return x > y ? x : y;
}

static int min(int x, int y) {
    return x < y ? x : y;
}

static unsigned get(void* p){
    return *(unsigned*)p;
}

static unsigned set(void* p, unsigned val){
    return *(unsigned*)p = val;
}

// size is aligned to 8-byte, so the lowest log_2-8 == 3 bits is zeros.
static unsigned pack(unsigned size, unsigned alloc){
    return size | alloc;
}

// Here p is the header or footer.
static unsigned unpack_size(void* p){
    return get(p) & ~0x7;
}

// Here p is the header or footer.
static unsigned unpack_alloc(void* p){
    return get(p) & 0x1;
}

// Here p is payload(effective) pointer
static void* get_hdr(void* p){
    return (char*)p - WSIZE;
}

static void* get_ftr(void* p){
    return (char*)p + unpack_size(get_hdr(p)) - DSIZE;
}

static void* pre_blk(void* p){
    return (char*)p - unpack_size((char*)p - DSIZE);
}

static void* next_blk(void* p ){
    return (char*)p + unpack_size((char*)p - WSIZE);
}

static void* coalesce(void* bp) {
    size_t size = unpack_size(get_hdr(bp));
    unsigned pre_alloc = unpack_alloc(get_ftr(pre_blk(bp)));
    unsigned next_alloc = unpack_alloc(get_hdr(next_blk(bp)));

    if(pre_alloc && next_alloc){
        return bp;
    } else if (pre_alloc) {
        size += unpack_size(get_hdr(next_blk(bp)));
    } else if (next_alloc) {
        size += unpack_size(get_hdr(pre_blk(bp)));
        bp = pre_blk(bp);
    } else {
        size += unpack_size(get_hdr(next_blk(bp)));
        size += unpack_size(get_hdr(pre_blk(bp)));
        bp = pre_blk(bp);
    }

    set(get_hdr(bp), pack(size, 0));
    set(get_ftr(bp), pack(size, 0));
    return bp;
}

static void* extend_heap(size_t words) {
    int size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    char *bp = mem_sbrk(size);

    if((long)bp == -1)
        return NULL;

    set(get_hdr(bp), pack(size, 0));
    set(get_ftr(bp), pack(size, 0));
    set(get_hdr(next_blk(bp)), pack(0, 1));

    return coalesce(bp);
}

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
    if((heap_listp = mem_sbrk(4 * WSIZE)) == (void*)-1)
        return -1;

    set(heap_listp, 0);
    set(heap_listp + 1 * WSIZE, pack(DSIZE, 1));
    set(heap_listp + 2 * WSIZE, pack(DSIZE, 1));
    set(heap_listp + 3 * WSIZE, pack(0, 1));

    heap_listp += 2 * WSIZE;

    if(extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;

    return 0;
}

static void* first_fit(size_t req_size){
    void*  bp = heap_listp;
    size_t blk_size = unpack_size(get_hdr(bp));

    while(blk_size){
        bp = next_blk(bp);
        blk_size = unpack_size(get_hdr(bp));
        if(blk_size >= req_size && !unpack_alloc(get_hdr(bp)))
            return bp;
    }

    return NULL;
}

static void place(void* bp, size_t req_size){
    size_t cur_size = unpack_size(get_hdr(bp));
    size_t rem_size = cur_size - req_size;

    if (rem_size < 2 * DSIZE) {
        set(get_hdr(bp), pack(cur_size, 1));
        set(get_ftr(bp), pack(cur_size, 1));
    } else {
        set(get_hdr(bp), pack(req_size, 1));
        set(get_ftr(bp), pack(req_size, 1));

        bp = next_blk(bp);

        set(get_hdr(bp), pack(rem_size, 0));
        set(get_ftr(bp), pack(rem_size, 0));
    }
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void* mm_malloc(size_t size) {
    if(!size)
        return NULL;

    int blk_size = ALIGN(size + DSIZE);
    char* bp = first_fit(blk_size);

    if (bp != NULL){
        place(bp, blk_size);
        return bp;
    }

    size_t extend_size = max(blk_size, CHUNKSIZE);
    bp = extend_heap(extend_size/WSIZE);

    if (bp != NULL){
        place(bp, blk_size);
        return bp;
    }

    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void* bp) {
    if(!bp)
        return;
    size_t size = unpack_size(get_hdr(bp));
    set(get_hdr(bp), pack(size, 0));
    set(get_ftr(bp), pack(size, 0));
    coalesce(bp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void* mm_realloc(void* ptr, size_t size) {
    if(!ptr)
        return mm_malloc(size);
    else if(!size){
        mm_free(ptr);
        return NULL;
    }

    void* newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;

    unsigned copy_size = min(size, unpack_size(get_hdr(ptr)));
    memcpy(newptr, ptr, copy_size);

    mm_free(ptr);
    return newptr;
}
