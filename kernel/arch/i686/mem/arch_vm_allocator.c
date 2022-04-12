#include <kernel/hal/processor.h>
#include <kernel/mem/page.h>

void *create_temp_phys_addr_mapping(uintptr_t) {
#ifdef CREATE_TEMP_PHYS_ADDR_MAPPING_CHECK
    if (bsp_enabled()) {
        assert(++get_current_processor()->arch_processor.phys_addr_mapping_count <= 2);
    }
#endif /* CREATE_TEMP_PHYS_ADDR_MAPPING_CHECK */
    return NULL;
}

void free_temp_phys_addr_mapping(void *ptr) {
    (void) ptr;

#ifdef CREATE_TEMP_PHYS_ADDR_MAPPING_CHECK
    if (bsp_enabled()) {
        get_current_processor()->arch_processor.phys_addr_mapping_count--;
    }
#endif /* CREATE_TEMP_PHYS_ADDR_MAPPING_CHECK */
}
