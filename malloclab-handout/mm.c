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
#include <assert.h>
#include <unistd.h>
#include <stdint.h>
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
// Why is 8? sizeof(size_t) depends on machine word size.
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


/* Constants and macros */
// Word & header/footer size in Byte
#define WSIZE 4
// Double words size in Byte
#define DSIZE 8
// Segregated Free List Length
#define CLASSNUM 20
// Extend heap size (4K Byte) when failed to find a proper free block
#define CHUNKSIZE (1<<12)



static inline unsigned max(unsigned x, unsigned y){
    return x > y ? x : y;
}

static inline unsigned min(unsigned x, unsigned y){
    return x < y ? x : y;
}


/* read & write a word at address p */
static inline unsigned get(void* p){
    return *(unsigned*)p;
}

static inline void set(void* p, unsigned val){
    *(unsigned*)p = val;
}

/* pack info including size & alloc sign bits */
static inline unsigned pack(unsigned size, unsigned alloc){
    return size | alloc;
}

/* get size and allocated bit of a block */
static inline unsigned unpack_size(void* p){
    return get(p) & ~0x7;
}

static inline unsigned unpack_alloc(void* p){
    return get(p) & 0x1;
}

/* compute address of header and footer from a given pointer of a block */
static inline void* get_hdrp(void* p){
    return (uint8_t*)p - WSIZE;
}

static inline void* get_ftrp(void* p){
    return (uint8_t*)p + unpack_size(get_hdrp(p)) - DSIZE;
}

/* compute address of next and previous block from a given pointer of a block */
static inline void* prev_blkp(void* p){
    return (uint8_t*)p - unpack_size((uint8_t*)p - DSIZE);
}

static inline void* next_blkp(void* p){
    return (uint8_t*)p + unpack_size((uint8_t*)p - WSIZE);
}

/* compute pointer address and content of next and previous free block from a given pointer of a free block, which is a list data structure. */
static inline void* prev_lstp(void* p){
    return p;
}

static inline void* next_lstp(void* p){
    return (uint8_t*)p + WSIZE;
}


static inline void* prev_lst(void* p){
    return (void*)get(p);
}

static inline void* next_lst(void* p){
    return (void*)get((uint8_t*)p + WSIZE);
}


static char *heap_listp;
static char *free_listp;
static void *coalesce(void *bp);
static void *extend_heap(size_t words);

static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);

static void insert_free_blk(void *bp);
static void remove_free_blk(void *bp);

static int get_list_idx(size_t asize);

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
    /* Create initial empty heap */
    /*
     *  * Heap structure:
     * +---------+---------+---------+-----------+
     * | ENTRY 0 | ENTRY 1 |   ...   | ENTRY k-1 |
     * +---------+---------+---------+-----------+
     * ^   4B        4B                   4B     ^
     * |                                         |
     * mem_heap_lo()/freeList                    freeList_end
     *
     * +-----------------+---------+---------+------------+-----------------+
     * | PROLOGUE HEADER | BLOCK 0 |   ...   | BLOCKS n-1 | EPILOGUE HEADER |
     * +-----------------+---------+---------+------------+-----------------+
     * ^        4B          ^                             ^       4B        ^
     * |                    |                             |                 |
     * freeList_end         heap_listp                    mem_heap_hi()     epilogue
     */

    if ((free_listp = mem_sbrk((CLASSNUM  + 4) * WSIZE)) == (void*)-1)
        return -1;

    for(int i = 0; i <  CLASSNUM; ++i){
        set(free_listp + i * WSIZE, 0);
    }

    set(free_listp + (CLASSNUM + 0) * WSIZE, 0);
    set(free_listp + (CLASSNUM + 1) * WSIZE, pack(DSIZE, 1));
    set(free_listp + (CLASSNUM + 2) * WSIZE, pack(DSIZE, 1));
    set(free_listp + (CLASSNUM + 3) * WSIZE, pack(0, 1));
    heap_listp = free_listp + (CLASSNUM + 2) * WSIZE;

    char* bp = extend_heap(CHUNKSIZE / WSIZE);
    if (bp == NULL)
        return -1;

    /* Set Null pointers*/
    // It seems only maybe due to repeatly call mm_init() to reset the content for the explicit free block list case.
    /*
     *PUT_PTR(PREV_PTR(bp), NULL);
     *PUT_PTR(NEXT_PTR(bp), NULL);
     */

    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void* mm_malloc(size_t size) {
    /* ignore spurious request */
    if (size == 0)
        return NULL;

    /* adjust size for alignment */
    size_t blk_size = DSIZE + ALIGN(size);
    assert(!(blk_size % 8));

    /* search free list for a fit */
    void* bp = find_fit(blk_size);
    if (bp != NULL) {
        place(bp, blk_size);
        return bp;
    }

    /* if not found from the free list, expanding heap if no fit found */
    bp = extend_heap(max(blk_size, CHUNKSIZE) / WSIZE);
    if (bp != NULL) {
        place(bp, blk_size);
        return bp;
    }
    return NULL;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void* bp) {
    if (bp == NULL)
        return;
    unsigned size = unpack_size(get_hdrp(bp));
    set(get_hdrp(bp), pack(size, 0));
    set(get_ftrp(bp), pack(size, 0));
    coalesce(bp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void* mm_realloc(void* ptr, size_t size) {
    if (ptr == NULL)
        return mm_malloc(size);
    if (size == 0){
        mm_free(ptr);
        return NULL;
    }

    // Here we can optimize.
    void* new_ptr = mm_malloc(size);
    size_t blk_size = min(size, unpack_size(get_hdrp(ptr)));

    memcpy(new_ptr, ptr, blk_size);
    mm_free(ptr);

    return new_ptr;
}

// Here argument is words number.
static void *extend_heap(size_t words) {
    size_t size = (words % 2)? (words + 1) * WSIZE: words * WSIZE;
    char *bp = mem_sbrk(size);
    if ((ssize_t)bp == -1)
        return NULL;

    set(get_hdrp(bp), pack(size, 0)); /* Header */
    set(get_ftrp(bp), pack(size, 0)); /* Footer */
    set(get_hdrp(next_blkp(bp)), pack(0, 1)); /* New epilogue header */
    /*
     *PUT_PTR(PREV_PTR(bp), NULL);
     *PUT_PTR(NEXT_PTR(bp), NULL);
     */

    return coalesce(bp);
}

static void *coalesce(void *bp) {
    void *prev_blk = prev_blkp(bp);
    void *next_blk = next_blkp(bp);
    size_t size = unpack_size(get_hdrp(bp));
    size_t prev_alloc = unpack_alloc(get_hdrp(prev_blk));
    size_t next_alloc = unpack_alloc(get_hdrp(next_blk));

    if (prev_alloc && next_alloc) { /* case 1 */ //prev and next block allocated
        insert_free_blk(bp);
        return bp;
    } else if (prev_alloc && (!next_alloc)) { /* case 2 */ // only prev block allocated, next block not allocated.
        size += unpack_size(get_hdrp(next_blk));
        remove_free_blk(next_blk);
    } else if ((!prev_alloc) && next_alloc) { /* case 3 */
        size += unpack_size(get_hdrp(prev_blk));
        remove_free_blk(prev_blk);
        bp = prev_blk;
    } else { /* case 4 */
        size += unpack_size(get_hdrp(next_blk));
        size += unpack_size(get_hdrp(prev_blk));
        remove_free_blk(next_blk);
        remove_free_blk(prev_blk);
        bp = prev_blk;
    }

    set (get_hdrp(bp), pack(size, 0));
    set (get_ftrp(bp), pack(size, 0));
    insert_free_blk(bp);
    return bp;
}

static int get_list_idx(size_t size) {
    /*
     *int index = 2;
     *size_t blk_size = 4;
     *while (size > blk_size) {
     *    index++;
     *    blk_size <<= 1;
     *}
     *assert(blk_size >= size);
     *return index > CLASSNUM ? CLASSNUM-1 : index-2;
     */
    for (int i = 4; i < CLASSNUM + 4; i++) {
        if (size <= (1 << i))
            return i - 4;
    }
    return CLASSNUM - 1;
}

static void *find_fit(size_t size) {
    for (int idx = get_list_idx(size); idx < CLASSNUM; idx++) {
        void* free_hdr = (void*)get(free_listp + idx * WSIZE);
        while (free_hdr != NULL){
            if (unpack_size(get_hdrp(free_hdr)) >= size)
                return free_hdr;
            free_hdr = next_lst(free_hdr);
        }
    }
    return NULL;

    /*
     *unsigned class_num = get_list_idx(size);
     *void*    bp;
     *while (class_num < CLASSNUM) {
     *    bp = free_listp + class_num * WSIZE;
     *    unsigned min_size = (unsigned)(-1);
     *    void*    min_bp = NULL;
     *    while (bp) {
     *        unsigned cur_size = unpack_size(get_hdrp(bp));
     *        if (cur_size >= size && cur_size < min_size) {
     *            min_size = cur_size;
     *            min_bp = bp;
     *        }
     *        bp = next_lst(bp);
     *    }
     *    if (min_bp) {
     *        return min_bp;
     *    }
     *    class_num++;
     *}
     *return NULL;
     */
}

static void place(void *bp, size_t req_size) {

    size_t total_size = unpack_size(get_hdrp(bp));
    size_t rem_size = total_size - req_size;

    remove_free_blk(bp);
    if (rem_size < 16) {
        set(get_hdrp(bp), pack(total_size, 1));
        set(get_ftrp(bp), pack(total_size, 1));
    }
    else {
        set(get_hdrp(bp), pack(req_size, 1));
        set(get_ftrp(bp), pack(req_size, 1));

        bp = next_blkp(bp);
        set(get_hdrp(bp), pack(rem_size, 0));
        set(get_ftrp(bp), pack(rem_size, 0));
        insert_free_blk(bp);
    }
}

/* remove free block from segragated free list in order of block size */
static void remove_free_blk(void *bp) {
    void *prev_free_lst = prev_lst(bp);
    void *next_free_lst = next_lst(bp);

    size_t size = unpack_size(get_hdrp(bp));
    int index = get_list_idx(size);

    if (!prev_free_lst && !next_free_lst) {     /* empty linked-list */
        set(free_listp + index * WSIZE, (unsigned)NULL);
    }else if (!prev_free_lst && next_free_lst) { /* first node */
        set(prev_lstp(next_free_lst), (unsigned)NULL);
        set(free_listp + index * WSIZE, (unsigned)next_free_lst);
    } else if (prev_free_lst && !next_free_lst) {          /* last node */
        set(next_lstp(prev_free_lst), (unsigned)NULL);
    } else {
        set(prev_lstp(next_free_lst), (unsigned)prev_free_lst);
        set(next_lstp(prev_free_lst), (unsigned)next_free_lst);
    }
}

/* insert free block into segragated free list in order of block size */
static void insert_free_blk(void *bp) {
    size_t size = unpack_size(get_hdrp(bp));
    int index = get_list_idx(size);
    void *root = (void*)get(free_listp + index * WSIZE);

    if(!root){
        set(prev_lstp(bp), (unsigned)NULL);
        set(next_lstp(bp), (unsigned)NULL);
    } else {
        set(prev_lstp(bp), (unsigned)NULL);
        set(next_lstp(bp), (unsigned)root);
        set(prev_lstp(root), (unsigned)bp);
    }

    set(free_listp + index * WSIZE, (unsigned)(bp));
}
