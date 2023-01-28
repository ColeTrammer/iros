#include <kernel/hal/processor.h>
#include <kernel/mem/page.h>

void vm_bootstrap_temp_page_mapping_processor(struct processor *) {}

// Physical addresses are already mapped in at VIRT_ADDR(MAX_PML4_ENTRIES - 3)
void *get_identity_phys_addr_mapping(uintptr_t phys_addr) {
    return (void *) (phys_addr + PHYS_ID_START);
}

void *create_temp_phys_addr_mapping(uintptr_t phys_addr) {
#ifdef CREATE_TEMP_PHYS_ADDR_MAPPING_CHECK
    if (bsp_enabled()) {
        assert(++get_current_processor()->arch_processor.phys_addr_mapping_count <= 2);
    }
#endif /* CREATE_TEMP_PHYS_ADDR_MAPPING_CHECK */
    return get_identity_phys_addr_mapping(phys_addr);
}

void free_temp_phys_addr_mapping(void *ptr) {
    (void) ptr;

#ifdef CREATE_TEMP_PHYS_ADDR_MAPPING_CHECK
    if (bsp_enabled()) {
        get_current_processor()->arch_processor.phys_addr_mapping_count--;
    }
#endif /* CREATE_TEMP_PHYS_ADDR_MAPPING_CHECK */
}
