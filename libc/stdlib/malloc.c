#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <kernel/mem/page.h>

#ifdef __is_libk
#include <kernel/mem/vm_allocator.h>

void *sbrk(intptr_t increment) {
    if (increment < 0) {
        void *res = add_vm_pages(0);
        remove_vm_pages(-increment);
        return res;
    }
    return add_vm_pages(increment);
}
#else
extern void *sbrk(intptr_t increment);
#endif

struct metadata {
    size_t prev_size;
    size_t size;
};

#define ALLOCATED (1)
#define SET_ALLOCATED(block) ((block)->size |= ALLOCATED)
#define CLEAR_ALLOCATED(block) ((block)->size &= ~ALLOCATED)
#define IS_ALLOCATED(block) ((block)->size & ALLOCATED)
#define GET_SIZE(block) ((block)->size & ~ALLOCATED)

#define NUM_PAGES(sz) (((sz) - 1) / PAGE_SIZE + 1)
#define NEW_BLOCK_SIZE(n) (2 * sizeof(struct metadata) + (n))
#define NEXT_BLOCK(block) ((struct metadata*) (((uintptr_t) (block)) + sizeof(struct metadata) + GET_SIZE((block))))
#define PREV_BLOCK(block) ((struct metadata*) (((uintptr_t) (block)) - sizeof(struct metadata) - (block)->prev_size))
#define GET_BLOCK(p) ((struct metadata*) (((uintptr_t) (p)) - sizeof(struct metadata)))

static struct metadata *start;
static uintptr_t heap_end;

void *calloc(size_t n, size_t sz) {
    void *p = malloc(n * sz);
    if (p == NULL) { return p; }
    memset(p, 0, sz);
    return p;
}

void *malloc(size_t n) {
    if (n == 0) {
        return NULL;
    }
    if (n & 1) {
        n++;
    }
    if (!start) {
        start = sbrk(NUM_PAGES(NEW_BLOCK_SIZE(n)));
        heap_end = ((uintptr_t) start) + NUM_PAGES(NEW_BLOCK_SIZE(n)) * PAGE_SIZE;
    }

    struct metadata *block = start;
    while (block->size != 0) {
        if (!IS_ALLOCATED(block) && GET_SIZE(block) >= n) {
            SET_ALLOCATED(block);
            return block + 1;
        }
        block = NEXT_BLOCK(block);
    }

    if (((uintptr_t) block) + NEW_BLOCK_SIZE(n) > heap_end) {
        sbrk(NUM_PAGES(((uintptr_t) block) + NEW_BLOCK_SIZE(n) - heap_end));
        heap_end += NUM_PAGES(((uintptr_t) block) + NEW_BLOCK_SIZE(n) - heap_end) * PAGE_SIZE;
    }

    block->size = n;
    SET_ALLOCATED(block);
    block = NEXT_BLOCK(block);
    block->prev_size = n;
    return PREV_BLOCK(block) + 1;
}

void *realloc(void *p, size_t sz) {
    void *new_p = malloc(sz);
    if (new_p == NULL) { return new_p; }
    memcpy(new_p, p, sz);
    free(p);
    return new_p;
}

void free(void *p) {
    if (p == NULL) {
        return;
    }
    struct metadata *block = GET_BLOCK(p);
    CLEAR_ALLOCATED(block);
}