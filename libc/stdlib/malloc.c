#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include <kernel/mem/page.h>

#define __MALLOC_MAGIG_CHECK 0x2A8F30B241BFA759UL

#ifdef __is_libk
#include <kernel/mem/vm_allocator.h>
#include <kernel/mem/vm_region.h>

void *sbrk(intptr_t increment) {
    if (increment < 0) {
        void *res = add_vm_pages_end(0, VM_KERNEL_HEAP);
        remove_vm_pages_end(-increment, VM_KERNEL_HEAP);
        return res;
    }
    return add_vm_pages_end(increment, VM_KERNEL_HEAP);
}
#else
#include <unistd.h>
#endif

struct metadata {
    size_t magic;
    size_t prev_size;
    size_t size;
} __attribute__((packed));

#define ALLOCATED (1)
#define SET_ALLOCATED(block) ((block)->size |= ALLOCATED)
#define CLEAR_ALLOCATED(block) ((block)->size &= ~ALLOCATED)
#define IS_ALLOCATED(block) ((block)->size & ALLOCATED)
#define GET_SIZE(block) ((block)->size & ~ALLOCATED)

#define NUM_PAGES_IN_LENGTH(sz) (((sz) - 1) / PAGE_SIZE + 1)
#define NEW_BLOCK_SIZE(n) (2 * sizeof(struct metadata) + (n))
#define NEXT_BLOCK(block) ((struct metadata*) (((uintptr_t) (block)) + sizeof(struct metadata) + GET_SIZE((block))))
#define PREV_BLOCK(block) ((struct metadata*) (((uintptr_t) (block)) - sizeof(struct metadata) - (block)->prev_size))
#define GET_BLOCK(p) ((struct metadata*) (((uintptr_t) (p)) - sizeof(struct metadata)))

static struct metadata *start;
static uintptr_t heap_end;

#ifdef __is_libk
#include <kernel/util/spinlock.h>

static spinlock_t heap_lock = SPINLOCK_INITIALIZER;
#endif /* __is_libk */

#if defined(__is_libk) && defined(KERNEL_MALLOC_DEBUG)
#include <kernel/hal/output.h>
#undef calloc
void *calloc(size_t n, size_t sz, int line, const char *func) {
    debug_log("Calloc: [ %s, %d ]\n", func, line);
#else
void *calloc(size_t n, size_t sz) {
#endif /* __is_libk && KERNEL_MALLOC_DEBUG */
    void *p = malloc(n * sz);
    if (p == NULL) { return p; }
    memset(p, 0, n * sz);
    return p;
}

#if defined(__is_libk) && defined(KERNEL_MALLOC_DEBUG)
#include <kernel/hal/output.h>
#undef realloc
void *realloc(void *p, size_t sz, int line, const char *func) {
    debug_log("Realloc: [ %s, %d ]\n", func, line);
#else
void *realloc(void *p, size_t sz) {
#endif /* __is_libk && KERNEL_MALLOC_DEBUG */
    void *new_p = malloc(sz);
    if (new_p == NULL) { return new_p; }
    memcpy(new_p, p, sz);
    free(p);
    return new_p;
}

#if defined(__is_libk) && defined(KERNEL_MALLOC_DEBUG)
#include <kernel/hal/output.h>
#undef free
void free(void *p, int line, const char *func) {
    debug_log("Free: [ %s, %d ]\n", func, line);

    struct metadata *_block = start;
    while (_block != NULL && _block->size != 0) {
        debug_log("Malloc block: [ %#.16lX ]\n", (uintptr_t) _block);
        assert(_block->magic == __MALLOC_MAGIG_CHECK);
        _block = NEXT_BLOCK(_block);
    }
#else
void free(void *p) {
#endif /* __is_libk && KERNEL_MALLOC_DEBUG */
    if (p == NULL) {
        return;
    }

#ifdef __is_libk
    spin_lock(&heap_lock);
#endif /* __is_libk */

    struct metadata *block = GET_BLOCK(p);
    assert(block->magic == __MALLOC_MAGIG_CHECK);
    assert(IS_ALLOCATED(block));
    CLEAR_ALLOCATED(block);

#if defined(KERNEL_MALLOC_DEBUG) && defined(__is_libk)
    debug_log("Malloc block freed: [ %#.16lX ]\n", (uintptr_t) block);
#endif /* KERNEL_MALLOC_DEBUG && __is_libk */

#ifdef __is_libk
    spin_unlock(&heap_lock);
#endif /* __is_libk */
}

#if defined(__is_libk) && defined(KERNEL_MALLOC_DEBUG)
#include <kernel/hal/output.h>
#undef malloc
void *malloc(size_t n, int line, const char *func) {
    debug_log("Malloc: [ %s, %d ]\n", func, line);

    struct metadata *_block = start;
    while (_block != NULL && _block->size != 0) {
        debug_log("Malloc block: [ %#.16lX ]\n", (uintptr_t) _block);
        assert(_block->magic == __MALLOC_MAGIG_CHECK);
        _block = NEXT_BLOCK(_block);
    }
#else
void *malloc(size_t n) {
#endif /* __is_libk && KERNEL_MALLOC_DEBUG */
    if (n == 0) {
        return NULL;
    }
    if (n % sizeof(uint64_t) != 0) {
        n = n - (n % sizeof(uint64_t)) + sizeof(uint64_t);
    }

#ifdef __is_libk
    spin_lock(&heap_lock);
#endif /* __is_libk */

    if (!start) {
        start = sbrk(NUM_PAGES_IN_LENGTH(NEW_BLOCK_SIZE(n)));
        heap_end = ((uintptr_t) start) + NUM_PAGES_IN_LENGTH(NEW_BLOCK_SIZE(n)) * PAGE_SIZE;
        start->prev_size = 0;
        start->size = n;
        SET_ALLOCATED(start);
        start->magic = __MALLOC_MAGIG_CHECK;

        start = NEXT_BLOCK(start);
        start->prev_size = n;
        start->magic = __MALLOC_MAGIG_CHECK;
        start = PREV_BLOCK(start);

#ifdef __is_libk
        spin_unlock(&heap_lock);
#endif /* __is_libk */

#if defined(KERNEL_MALLOC_DEBUG) && defined(__is_libk)
        debug_log("Malloc block allocated: [ %#.16lX, %d ]\n", (uintptr_t) start, __LINE__);
#endif /* KERNEL_MALLOC_DEBUG && __is_libk */

        return start + 1;
    }

    struct metadata *block = start;
    while (block->size != 0) {
        assert(block->magic == __MALLOC_MAGIG_CHECK);
        if (!IS_ALLOCATED(block) && GET_SIZE(block) >= n) {
            SET_ALLOCATED(block);

#ifdef __is_libk
            spin_unlock(&heap_lock);
#endif /* __is_libk */


#if defined(KERNEL_MALLOC_DEBUG) && defined(__is_libk)
        debug_log("Malloc block allocated: [ %#.16lX, %d ]\n", (uintptr_t) block, __LINE__);
#endif /* KERNEL_MALLOC_DEBUG && __is_libk */
            return block + 1;
        }
        block = NEXT_BLOCK(block);
    }

    if (((uintptr_t) block) + NEW_BLOCK_SIZE(n) > heap_end) {
        sbrk(NUM_PAGES_IN_LENGTH(((uintptr_t) block) + NEW_BLOCK_SIZE(n) - heap_end));
        heap_end += NUM_PAGES_IN_LENGTH(((uintptr_t) block) + NEW_BLOCK_SIZE(n) - heap_end) * PAGE_SIZE;
    }

    block->size = n;
    SET_ALLOCATED(block);
    block = NEXT_BLOCK(block);
    block->prev_size = n;
    block->magic = __MALLOC_MAGIG_CHECK;
    struct metadata *ret = PREV_BLOCK(block) + 1;

#if defined(KERNEL_MALLOC_DEBUG) && defined(__is_libk)
    debug_log("Malloc block allocated: [ %#.16lX, %d ]\n", (uintptr_t) block, __LINE__);
#endif /* KERNEL_MALLOC_DEBUG && __is_libk */

#ifdef __is_libk
    spin_unlock(&heap_lock);
#endif /* __is_libk */

    return ret;
}