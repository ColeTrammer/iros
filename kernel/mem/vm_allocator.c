#include <sys/types.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <kernel/hal/x86_64/drivers/vga.h>
#include <kernel/mem/kernel_vm.h>
#include <kernel/mem/page.h>
#include <kernel/mem/page_frame_allocator.h>
#include <kernel/mem/vm_region.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/proc/task.h>
#include <kernel/hal/output.h>
#include <kernel/util/spinlock.h>

static struct vm_region *kernel_vm_list = NULL;
#if ARCH==X86_64
static struct vm_region kernel_phys_id;
#endif /* ARCH==X86_64 */
static struct vm_region kernel_text;
static struct vm_region kernel_rod;
static struct vm_region kernel_data;
static struct vm_region kernel_heap;
static struct vm_region initrd;
static spinlock_t kernel_vm_lock = SPINLOCK_INITIALIZER;

void init_vm_allocator(uintptr_t initrd_phys_start, uintptr_t initrd_phys_end) {
#if ARCH==X86_64
    kernel_phys_id.start = VIRT_ADDR(MAX_PML4_ENTRIES - 3, 0, 0, 0);
    kernel_phys_id.end = kernel_phys_id.start + get_total_phys_memory();
    kernel_phys_id.flags = VM_NO_EXEC | VM_GLOBAL | VM_WRITE;
    kernel_phys_id.type = VM_KERNEL_PHYS_ID;
    kernel_vm_list = add_vm_region(kernel_vm_list, &kernel_phys_id);
    assert(kernel_vm_list == &kernel_phys_id);
#endif /* ARCH==X86_64 */

    kernel_text.start = KERNEL_VM_START & ~0xFFF;
    kernel_text.end = kernel_text.start + NUM_PAGES(KERNEL_TEXT_START, KERNEL_TEXT_END) * PAGE_SIZE;
    kernel_text.flags = VM_GLOBAL;
    kernel_text.type = VM_KERNEL_TEXT;
    kernel_vm_list = add_vm_region(kernel_vm_list, &kernel_text);

    kernel_rod.start = kernel_text.end;
    kernel_rod.end = kernel_rod.start + NUM_PAGES(KERNEL_ROD_START, KERNEL_ROD_END) * PAGE_SIZE;
    kernel_rod.flags = VM_GLOBAL | VM_NO_EXEC;
    kernel_rod.type = VM_KERNEL_ROD;
    kernel_vm_list = add_vm_region(kernel_vm_list, &kernel_rod);

    kernel_data.start = kernel_rod.end;
    kernel_data.end = kernel_data.start + NUM_PAGES(KERNEL_DATA_START, KERNEL_BSS_END) * PAGE_SIZE;
    kernel_data.flags = VM_WRITE | VM_GLOBAL | VM_NO_EXEC;
    kernel_data.type = VM_KERNEL_DATA;
    kernel_vm_list = add_vm_region(kernel_vm_list, &kernel_data);

    initrd.start = kernel_data.end;
    initrd.end = ((initrd.start + initrd_phys_end - initrd_phys_start) & ~0xFFF) + PAGE_SIZE;
    initrd.flags = VM_GLOBAL | VM_NO_EXEC;
    initrd.type = VM_INITRD;
    kernel_vm_list = add_vm_region(kernel_vm_list, &initrd);
    for (int i = 0; initrd.start + i < initrd.end; i += PAGE_SIZE) {
        map_phys_page(initrd_phys_start + i, initrd.start + i, initrd.flags);
    }

    kernel_heap.start = initrd.end;
    kernel_heap.end = kernel_heap.start;
    kernel_heap.flags = VM_WRITE | VM_GLOBAL | VM_NO_EXEC;
    kernel_heap.type = VM_KERNEL_HEAP;
    kernel_vm_list = add_vm_region(kernel_vm_list, &kernel_heap);

    clear_initial_page_mappings();

    uintptr_t new_structure = create_paging_structure(kernel_vm_list, true);
    load_paging_structure(new_structure);

#if ARCH==X86_64
    create_phys_id_map();
#endif /* ARCH==X86_64 */

    debug_log("Finished Initializing VM Allocator\n");
}

void *add_vm_pages_end(size_t n, uint64_t type) {
    struct vm_region *list;
    if (type > VM_KERNEL_HEAP) {
        list = get_current_task()->task_memory;
    } else {
        list = kernel_vm_list;
        spin_lock(&kernel_vm_lock);
    }

    struct vm_region *region = get_vm_region(list, type);

    uintptr_t old_end = region->end;
    if (extend_vm_region_end(list, type, n) < 0) {
        return NULL; // indicate there is no room
    }
    for (size_t i = 0; i < n; i++) {
        map_page(old_end + i * PAGE_SIZE, region->flags);
    }

    if (type <= VM_KERNEL_HEAP) {
        spin_unlock(&kernel_vm_lock);
    }
    
    memset((void*) old_end, 0, n * PAGE_SIZE);
    return (void*) old_end;
}

void *add_vm_pages_start(size_t n, uint64_t type) {
    struct vm_region *list;
    if (type > VM_KERNEL_HEAP) {
        list = get_current_task()->task_memory;
    } else {
        list = kernel_vm_list;
        spin_lock(&kernel_vm_lock);
    }

    struct vm_region *region = get_vm_region(list, type);
    uintptr_t old_start = region->start;
    if (extend_vm_region_start(list, type, n) < 0) {
        return NULL; // indicate there is no room
    }
    for (size_t i = 1; i <= n; i++) {
        map_page(old_start - i * PAGE_SIZE, region->flags);
    }

    if (type <= VM_KERNEL_HEAP) {
        spin_unlock(&kernel_vm_lock);
    }
    
    memset((void*) (old_start - n * PAGE_SIZE), 0, n * PAGE_SIZE);
    return (void*) old_start;
}

void remove_vm_pages_end(size_t n, uint64_t type) {
    struct vm_region *list;
    if (type > VM_KERNEL_HEAP) {
        list = get_current_task()->task_memory;
    } else {
        list = kernel_vm_list;
        spin_lock(&kernel_vm_lock);
    }

    uintptr_t old_end = get_vm_region(list, type)->end;
    if (contract_vm_region_end(list, type, n) < 0) {
        printf("%s\n", "Error: Removed to much memory");
        abort();
    }

    if (type <= VM_KERNEL_HEAP) {
        spin_unlock(&kernel_vm_lock);
    }

    for (size_t i = 1; i <= n; i++) {
        unmap_page(old_end - i * PAGE_SIZE);
    }
}

void *map_file(off_t length, uint64_t flags) {
    struct vm_region **list = &get_current_task()->task_memory;
    struct vm_region *last_file = get_vm_last_region(*list, VM_PROCESS_FILE);

    struct vm_region *to_add = calloc(1, sizeof(struct vm_region));
    to_add->type = VM_PROCESS_FILE;
    to_add->flags = flags;
    to_add->start = last_file->end;
    to_add->end = ((to_add->start + length) & ~0xFFF) + PAGE_SIZE;
    *list = add_vm_region(*list, to_add);

    map_vm_region(to_add);

    return (void*) to_add->start;
}

void remove_vm_pages_start(size_t n, uint64_t type) {
    struct vm_region *list;
    if (type > VM_KERNEL_HEAP) {
        list = get_current_task()->task_memory;
    } else {
        list = kernel_vm_list;
        spin_lock(&kernel_vm_lock);
    }

    uintptr_t old_start = get_vm_region(list, type)->start;
    if (contract_vm_region_start(list, type, n) < 0) {
        printf("%s\n", "Error: Removed to much memory");
        abort();
    }

    if (type <= VM_KERNEL_HEAP) {
        spin_unlock(&kernel_vm_lock);
    }

    for (size_t i = 0; i < n; i++) {
        unmap_page(old_start + i * PAGE_SIZE);
    }
}

struct vm_region *find_first_kernel_vm_region() {
    struct vm_region *list = kernel_vm_list;
    struct vm_region *first = list;
    while (list != NULL) {
        if (list->start < first->start) {
            first = list;
        }

        list = list->next;
    }

    return first;
}

struct vm_region *find_vm_region(uint64_t type) {
    struct vm_region *list;
    if (type > VM_KERNEL_HEAP) {
        list = get_current_task()->task_memory;
    } else {
        list = kernel_vm_list;
        spin_lock(&kernel_vm_lock);
    }

    struct vm_region *region = get_vm_region(list, type);

    if (type <= VM_KERNEL_HEAP) {
        spin_unlock(&kernel_vm_lock);
    }

    return region;
}

struct vm_region *find_vm_region_by_addr(uintptr_t addr) {
    struct vm_region *region = get_current_task()->task_memory;

    while (region) {
        if (region->start <= addr && addr <= region->end) {
            return region;
        }
        region = region->next;
    }

    region = kernel_vm_list;
    while (region) {
        if (region->start <= addr && addr <= region->end) {
            return region;
        }
        region = region->next;
    }

    return NULL;
}

struct vm_region *clone_task_vm() {
    struct vm_region *list = get_current_task()->task_memory;
    struct vm_region *new_list = NULL;
    struct vm_region *region = list;

    while (region != NULL) {
        struct vm_region *to_add = calloc(1, sizeof(struct vm_region));
        memcpy(to_add, region, sizeof(struct vm_region));

        if (to_add->type == VM_DEVICE_MEMORY_MAP_DONT_FREE_PHYS_PAGES) {
            struct inode *inode = to_add->backing_inode;
            spin_lock(&inode->lock);
            inode->ref_count++;
            spin_unlock(&inode->lock);
        }

        new_list = add_vm_region(new_list, to_add);
        region = region->next;
    }

    return new_list;
}