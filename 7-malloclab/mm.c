#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "memlib.h"
#include "mm.h"

team_t team = {
    /* Team name */
    "malloc bug",
    /* First member's full name */
    "Lixiaoqi",
    /* First member's email address */
    "lxq@mail.ustc.edu.cn",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    "",
};

/* #define __DEBUG__ */
/* #define __DEBUG_ASSERT__  */

/* Block tructure:
 * low-addr --> high-addr
 *
 * Using:
 * |--Foot--*--Payload--|--Head--|
 *
 * Free:
 * |--Foot--*--Prev--|--Succ--|--free--|--Head--|
 */
typedef struct Border {
    size_t size;
    /* @bit0: if this block is free
     */
    unsigned char info;

} Head, Foot;

#define ALIGNMENT 8
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define ALIGNED(number) ((number & 0x7) == 0)
#define BORDER_SIZE (ALIGN(sizeof(struct Border)))
#define MIN_PAYLOAD_SIZE (ALIGN(2 * sizeof(void*)))

#define max(x, y) (x > y ? x : y)

/* global variables */
void* free_list = NULL;

/* helper functions */
inline Foot* get_foot(void* ptr) { return ptr - BORDER_SIZE; }
inline Head* get_head(void* ptr)
{
    size_t size = get_foot(ptr)->size;
#ifdef __DEBUG_ASSERT__
    assert(ALIGNED(size));
    assert(ALIGNED((unsigned)ptr));
#endif
    return ptr + size;
}
inline size_t get_size(void* ptr) { return get_foot(ptr)->size; }
inline unsigned char is_free(void* ptr) { return (get_foot(ptr)->info & 0x1); }

inline int is_border_free(struct Border* border) { return border->info & 0x1; }
inline void set_border_used(struct Border* border) { border->info &= ~0x1; }
inline void set_border_free(struct Border* border) { border->info |= 0x1; }

inline void set_size(void* ptr, size_t size)
{
#ifdef __DEBUG_ASSERT__
    assert(ALIGNED(size));
#endif

    get_foot(ptr)->size = size;
    get_head(ptr)->size = size;
}
inline void set_used(void* ptr)
{
    set_border_used(get_foot(ptr));
    set_border_used(get_head(ptr));
}
inline void set_free(void* ptr)
{
    set_border_free(get_foot(ptr));
    set_border_free(get_head(ptr));
}

inline void** get_prev_ref(void* ptr) { return ptr; }
inline void** get_succ_ref(void* ptr) { return ptr + sizeof(void*); }
inline void* get_prev(void* ptr) { return *get_prev_ref(ptr); }
inline void* get_succ(void* ptr) { return *get_succ_ref(ptr); }
inline void set_prev(void* ptr, void* value) { *get_prev_ref(ptr) = value; }
inline void set_succ(void* ptr, void* value) { *get_succ_ref(ptr) = value; }

void debug_border_info(struct Border* bd, char* name)
{
    printf("\t%s %p, with size %u, free %d\n", name, bd, bd->size, is_border_free(bd));
}
void debug_list_info()
{
    if (free_list) {
        printf("free_list: ");
        for (void* ptr = free_list; ptr; ptr = get_succ(ptr)) {
            printf("->%p", ptr);
        }
        printf("\n");
    } else {
        printf("free_list is empty\n");
    }
}
void debug_block_info(void* ptr)
{
    Head* head = get_head(ptr);
    Foot* foot = get_foot(ptr);
    printf("[debug info for block with ptr %p]\n", ptr);
    debug_border_info(head, "\thead");
    debug_border_info(foot, "\tfoot");
}

void add_to_free_list(void* ptr)
{
    // insert to the front of the list
    set_prev(ptr, NULL);
    if (free_list) {
        set_prev(free_list, ptr);
        set_succ(ptr, free_list);
    } else {
        set_succ(ptr, NULL);
    }
    free_list = ptr;
}

void remove_from_list(void* ptr)
{
    if (get_prev(ptr)) {
        set_succ(get_prev(ptr), get_succ(ptr));
    } else {
#ifdef __DEBUG_ASSERT__
        assert(ptr == free_list);
#endif
        free_list = get_succ(ptr);
    }

    if (get_succ(ptr)) {
        set_prev(get_succ(ptr), get_prev(ptr));
    } else {
    }
}

// find first fit
void* find_fit(size_t size)
{
    for (void* ptr = free_list; ptr != NULL; ptr = get_succ(ptr)) {
        if (get_size(ptr) >= size)
            return ptr;
    }
    return NULL;
}

void* coalesce(void* ptr)
{
#ifdef __DEBUG_ASSERT__
    assert(is_border_free(get_head(ptr)));
    assert(is_border_free(get_foot(ptr)));
#endif
    size_t cur_size = get_size(ptr);
    Head* prev_head = ptr - 2 * BORDER_SIZE;
    Foot* next_foot = (void*)get_head(ptr) + BORDER_SIZE;

    int prev_free = is_border_free(prev_head);
    int next_free = is_border_free(next_foot);

    void* prev_ptr = ptr - 2 * BORDER_SIZE - prev_head->size;
    void* next_ptr = (void*)get_head(ptr) + 2 * BORDER_SIZE;

    size_t new_size;
    void* new_ptr;

    if (prev_free && next_free) {
        new_size = cur_size + prev_head->size + next_foot->size + 4 * BORDER_SIZE;
        new_ptr = prev_ptr;
        remove_from_list(prev_ptr);
        remove_from_list(next_ptr);
    } else if (prev_free) {
        new_size = cur_size + prev_head->size + 2 * BORDER_SIZE;
        new_ptr = prev_ptr;
        remove_from_list(prev_ptr);
    } else if (next_free) {
        new_size = cur_size + next_foot->size + 2 * BORDER_SIZE;
        new_ptr = ptr;
        remove_from_list(next_ptr);
    } else
        return ptr;

    set_size(new_ptr, new_size);
#ifdef __DEBUG_ASSERT__
    assert(ALIGNED(new_size));
    assert(is_border_free(get_head(new_ptr)));
    assert(is_border_free(get_foot(new_ptr)));
#endif
    return coalesce(new_ptr);
}

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
#ifdef __DEBUG__
    printf("mm_init()\n");
    printf("BORDER_SIZE=%u\n", BORDER_SIZE);
#endif
    void* ptr = mem_sbrk(ALIGN(2 * BORDER_SIZE));

    set_size(ptr, 0);
    set_used(ptr + BORDER_SIZE);

    free_list = NULL;

    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void* mm_malloc(size_t size)
{
#ifdef __DEBUG__
    printf("mm_malloc(%u)\n", size);
#endif
    // no less than min size of payload
    size = ALIGN(max(size, MIN_PAYLOAD_SIZE));
    void* ptr = find_fit(size);
    if (ptr) {
        size_t ori_size = get_size(ptr);

        // check if the free block is too large
        if (ori_size - size >= MIN_PAYLOAD_SIZE + 2 * BORDER_SIZE) {
            void* cut_ptr = ptr + size + 2 * BORDER_SIZE;
            // deal with new free block
            set_size(cut_ptr, ori_size - size - 2 * BORDER_SIZE);
            set_free(cut_ptr);
            add_to_free_list(cut_ptr);
            // required block
            set_size(ptr, size);
        }
        set_used(ptr);
        remove_from_list(ptr);
#ifdef __DEBUG__
        printf("\treturn free block %p\n", ptr);
        debug_block_info(ptr);
#endif
        return ptr;
    }

    size_t new_size = ALIGN(size + 2 * BORDER_SIZE);
    ptr = mem_sbrk(new_size);
    if (ptr == (void*)-1)
        return NULL;
    else {
        set_size(ptr, size);
        set_used(ptr);

        // sentinel
        set_border_used(ptr + size + BORDER_SIZE);

#ifdef __DEBUG__
        printf("\treturn new block %p\n", ptr);
        debug_block_info(ptr);
#endif
        return ptr;
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void* ptr)
{
#ifdef __DEBUG__
    printf("mm_free(%p)\n", ptr);
#endif
    set_free(ptr);
    ptr = coalesce(ptr);
    add_to_free_list(ptr);
#ifdef __DEBUG__
    debug_list_info();
#endif
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void* mm_realloc(void* ptr, size_t size)
{
#ifdef __DEBUG__
    printf("mm_realloc(%p, %u)\n", ptr, size);
#endif
    void* oldptr = ptr;
    void* newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    copySize = *(size_t*)((char*)oldptr - SIZE_T_SIZE);
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}
