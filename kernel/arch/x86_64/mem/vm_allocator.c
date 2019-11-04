#include <kernel/mem/page.h>

// Physical addresses are already mapped in at VIRT_ADDR(MAX_PML4_ENTRIES - 3)
void *create_phys_addr_mapping(uintptr_t phys_addr) {
    return (void*) (phys_addr + VIRT_ADDR(MAX_PML4_ENTRIES - 3, 0, 0, 0));
}