#include <stdatomic.h>
#include <stdlib.h>

#include <kernel/mem/page_frame_allocator.h>
#include <kernel/mem/phys_page.h>
#include <kernel/proc/task.h>

struct phys_page *allocate_phys_page(void) {
    uintptr_t phys_addr = get_next_phys_page(get_current_task()->process);
    assert(phys_addr);

    // FIXME: This should be allocated using a more efficent allocator.
    struct phys_page *page = malloc(sizeof(struct phys_page));
    assert(page);
    page->ref_count = 1;
    page->phys_addr = phys_addr;

    return page;
}

struct phys_page *bump_phys_page(struct phys_page *page) {
    int old_count = atomic_fetch_add(&page->ref_count, 1);
    assert(old_count >= 1);

    return page;
}

void drop_phys_page(struct phys_page *page) {
    if (atomic_fetch_sub(&page->ref_count, 1) == 1) {
        free_phys_page(page->phys_addr, get_current_task()->process);
        free(page);
    }
}