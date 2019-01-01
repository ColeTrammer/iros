#include <stddef.h>
#include <stdint.h>

#include <kernel/mem/page.h>
#include <kernel/mem/vm_region.h>

struct vm_region *add_vm_region(struct vm_region *list, struct vm_region *to_add) {
    struct vm_region **link = &list;
    while (*link != NULL && (*link)->start < to_add->end) {
        link = &(*link)->next;
    }
    to_add->next = *link;
    *link = to_add;
    return list;
}

struct vm_region *free_vm_region(struct vm_region *list, uint64_t start) {
    struct vm_region **link = &list;
    while (*link != NULL && (*link)->start != start) {
        link = &(*link)->next;
    }
    *link = (*link)->next;
    return list;
}

struct vm_region *get_vm_region(struct vm_region *list, uint64_t start) {
    while (list->start != start) {
        list = list->next;
    }
    return list;
}

int extend_vm_region(struct vm_region *list, uint64_t start, size_t num_pages) {
    while (list->start != start) {
        list = list->next;
    }
    uint64_t new_end = list->end + num_pages * PAGE_SIZE;
    if (new_end > list->next->start) {
        return -1; // Indicate there is no room
    }
    list->end = new_end;
    return 0;
}

int contract_vm_region(struct vm_region *list, uint64_t start, size_t num_pages) {
    while (list->start != start) {
        list = list->next;
    }
    uint64_t new_end = list->end - num_pages * PAGE_SIZE;
    if (new_end < list->start) {
        return -1; // Indicate took away too much
    }
    list->end = new_end;
    return 0;
}