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
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "memlib.h"
#include "mm.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {"lily (flower)", "liuly", "liuly@mail.ustc.edu.cn", "", ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

/* rounds up to the nearest multiple of ALIGNMENT */
/* 8 bytes in my computer */
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1 << 12)

/* 总是指向序言块的第二块 */
static char* free_listp;

/* util function for calculate max */
static inline unsigned max(unsigned x, unsigned y) {
    return x > y ? x : y;
}
static inline unsigned min(unsigned x, unsigned y) {
    return x < y ? x : y;
}

/* get or set p as unsigned pointer */
static inline unsigned get(void* p) {
    return *(unsigned*)p;
}
static inline void set(void* p, unsigned val) {
    *(unsigned*)p = val;
}

/* pack info. size & alloc sign bits */
static inline unsigned pack(unsigned size, unsigned alloc) {
    return size | alloc;
}
/* depack size and alloc sign bits from pack */
static inline unsigned unpack_size(void* p) {
    return get(p) & ~0x7;
}
static inline unsigned unpack_alloc(void* p) {
    return get(p) & 1;
}

/*compute address of heade and footer from a pointer*/
static inline void* get_hdrp(void* bp) {
    return bp - WSIZE;
}
static inline void* get_ftrp(void* bp) {
    return bp + unpack_size(get_hdrp(bp)) - DSIZE;
}

/* 根据有效载荷指针, 找到头部，脚部，前一块，下一块 */
static inline void* prev_blkp(void* bp) {
    return bp - unpack_size(bp - DSIZE);
}
static inline void* next_blkp(void* bp) {
    return bp + unpack_size(bp - WSIZE);
}

/* find pre or suc pointer for given bp */
static inline void* prev_lst(void* bp) {
    return (void*)get(bp);
}
static inline void* next_lst(void* bp) {
    return (void*)(get((void*)bp + WSIZE));
}

static inline void* prev_lstp(void* p){
    return p;
}

static inline void* next_lstp(void* p){
    return (uint8_t*)p + WSIZE;
}

/* find head pointer for given class_num */
static inline void* get_head(int class_num) {
    return (void*)(get(free_listp + WSIZE * class_num));
}

#define CLASSNUM 20

static void*    extend_heap(unsigned words);      // 扩展堆
static void*    coalesce(void* bp);                  // 合并空闲块
static void*    find_fit(unsigned asize);         // 找到匹配的块
static unsigned get_list_index(unsigned size);    // 找到对应大小类
static void     place(void* bp, unsigned asize);  // 分割空闲块
static void remove_free_blk(void* bp);  // 从相应链表中删除块
static void insert_free_blk(void* bp);   // 在对应链表中插入块

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
    /* 申请四个字节空间 */
    if ((free_listp = mem_sbrk((4 + CLASSNUM) * WSIZE)) == (void*)-1)
        return -1;
    /* 初始化大小类头指针 */
    for (int i = 0; i < CLASSNUM; i++) {
        set(free_listp + i * WSIZE, 0);
    }
    /* 对齐 */
    set(free_listp + (CLASSNUM + 0) * WSIZE, 0);
    set(free_listp + (CLASSNUM + 1) * WSIZE, pack(DSIZE, 1));
    set(free_listp + (CLASSNUM + 2) * WSIZE, pack(DSIZE, 1));
    set(free_listp + (CLASSNUM + 3) * WSIZE, pack(0, 1));

    /* 扩展空闲空间 */
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void* mm_malloc(unsigned size) {
    if (size == 0)
        return NULL;

    unsigned asize;
    if (size <= DSIZE)
        asize = 2 * DSIZE;
    else
        asize = DSIZE + ALIGN(size);
        //asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);

    /* 寻找合适的空闲块 */
    void* bp = find_fit(asize);
    if (bp != NULL) {
        place(bp, asize);
        return bp;
    }
    /* 找不到则扩展堆 */
    bp = extend_heap(max(asize, CHUNKSIZE) / WSIZE);
    if (bp != NULL) {
        place(bp, asize);
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
    if(!new_ptr)
        return NULL;

    size_t blk_size = min(size, unpack_size(get_hdrp(ptr)));

    memcpy(new_ptr, ptr, blk_size);
    mm_free(ptr);

    return new_ptr;
}

void* extend_heap(unsigned words) {
    unsigned size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;

    void* bp = mem_sbrk(size); /* 指向有效载荷 */
    if ((ssize_t)(bp) == -1)
        return NULL;

    /* 设置头部和脚部 */
    set(get_hdrp(bp), pack(size, 0));         /* 空闲块头 */
    set(get_ftrp(bp), pack(size, 0));         /* 空闲块脚 */
    set(get_hdrp(next_blkp(bp)), pack(0, 1)); /* 片的新结尾块 */

    /* 判断相邻块是否是空闲块, 进行合并 */
    return coalesce(bp);
}

static void* coalesce(void* bp) {
    void*  prev_blk = prev_blkp(bp);
    void*  next_blk = next_blkp(bp);
    size_t size = unpack_size(get_hdrp(bp));
    size_t prev_alloc = unpack_alloc(get_ftrp(prev_blk));
    size_t next_alloc = unpack_alloc(get_hdrp(next_blk));

    if (prev_alloc && next_alloc) {
        insert_free_blk(bp);
        return bp;
    } else if (prev_alloc && (!next_alloc)) {
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

    set(get_hdrp(bp), pack(size, 0));
    set(get_ftrp(bp), pack(size, 0));
    insert_free_blk(bp);
    return bp;
}

unsigned get_list_index(unsigned size) {
    /*
     *for (int i = 4; i < CLASSNUM + 4; i++) {
     *    if (size <= (1 << i))
     *        return i - 4;
     *}
     *return CLASSNUM - 1;
     */
    int index = 2;
    size_t blk_size = 4;
    while (size > blk_size) {
        index++;
        blk_size <<= 1;
    }
    assert(blk_size >= size);
    return index > CLASSNUM ? CLASSNUM-1 : index-2;
}

void* find_fit(unsigned asize) {
    /*
     *for (int idx = get_list_idx(size); idx < CLASSNUM; idx++) {
     *    void* free_hdr = (void*)get(free_listp + idx * WSIZE);
     *    while (free_hdr != NULL){
     *        if (unpack_size(get_hdrp(free_hdr)) >= size)
     *            return free_hdr;
     *        free_hdr = next_lst(free_hdr);
     *    }
     *}
     *return NULL;
     */
    unsigned class_num = get_list_index(asize);
    void*    bp;
    /* 如果找不到合适的块，那么就搜索下一个更大的大小类 */
    while (class_num < CLASSNUM) {
        bp = get_head(class_num);
        /* 不为空则寻找 */
        unsigned min_size = (unsigned)(-1);
        void*    min_bp = NULL;
        while (bp) {
            unsigned cur_size = unpack_size(get_hdrp(bp));
            if (cur_size >= asize && cur_size < min_size) {
                min_size = cur_size;
                min_bp = bp;
            }
            /* 用后继找下一块 */
            bp = next_lst(bp);
        }
        /* 找不到则进入下一个大小类 */
        if (min_bp) {
            return min_bp;
        }
        class_num++;
    }
    return NULL;
}

void place(void* bp, unsigned req_size) {
    unsigned total_size = unpack_size(get_hdrp(bp));
    unsigned rem_size = total_size - req_size;

    /* 已分配，从空闲链表中删除 */
    remove_free_blk(bp);
    if (rem_size >= 2 * DSIZE) {
        set(get_hdrp(bp), pack(req_size, 1));
        set(get_ftrp(bp), pack(req_size, 1));
        // Can't be here, It's a totally big bug.
        //remove_free_blk(bp);
        /* bp 指向空闲块 */
        bp = next_blkp(bp);
        set(get_hdrp(bp), pack(rem_size, 0));
        set(get_ftrp(bp), pack(rem_size, 0));
        /* 加入分离出来的空闲块 */
        insert_free_blk(bp);
    }
    /* 设置为填充 */
    else {
        set(get_hdrp(bp), pack(total_size, 1));
        set(get_ftrp(bp), pack(total_size, 1));
        // Can't be here, It's a totally big bug.
        //remove_free_blk(bp);
    }
}

void insert_free_blk(void* bp) {
    /* 块大小 */
    unsigned size = unpack_size(get_hdrp(bp));
    /* 根据块大小找到头节点位置 */
    unsigned class_num = get_list_index(size);
    /* 空的，直接放 */
    if (get_head(class_num) == NULL) {
        set(bp, 0);
        set((unsigned*)bp + 1, 0);

        set(free_listp + WSIZE * class_num, (unsigned)bp);
    } else {
        set(bp, 0);
        set((unsigned*)bp + 1, (unsigned)get_head(class_num));

        set(get_head(class_num), (unsigned)bp);
        set(free_listp + WSIZE * class_num, (unsigned)bp);
    }
}

void remove_free_blk (void* bp) {
    unsigned size = unpack_size(get_hdrp(bp));
    unsigned class_num = get_list_index(size);

    void *pre_p = prev_lst(bp), *suc_p = next_lst(bp);
    /* 唯一节点 头节点设为 null */
    if (!pre_p && !suc_p) {
        set(free_listp + WSIZE * class_num, 0);
    }
    /* 最后一个节点 前驱的后继设为 null */
    else if (pre_p && !suc_p) {
        set(pre_p + WSIZE, 0);
    }
    /* 第一个结点 头节点设为 bp 的后继 */
    else if (!pre_p && suc_p) {
        set(free_listp + WSIZE * class_num, (unsigned)suc_p);
        set(suc_p, 0);
    }
    /* 中间结点，前驱的后继设为后继，后继的前驱设为前驱 */
    else {
        set(pre_p + WSIZE, (unsigned)suc_p);
        set(suc_p, (unsigned)pre_p);
    }
}
