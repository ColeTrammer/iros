#include <kernel/hal/processor.h>
#include <kernel/mem/page.h>

static __attribute__((aligned(0x2000))) char __temp_page_mappings[0x2000];
static __attribute__((aligned(0x1000))) char __temp_page_mappings_pt_storage[0x1000];

static uint32_t *bsp_temp_page_mapping_pt = (void *) __temp_page_mappings_pt_storage;
static uint32_t bsp_temp_page_mapping_count;

void vm_bootstrap_temp_page_mapping() {
    uint32_t virt_addr = (uintptr_t) __temp_page_mappings_pt_storage;
    uint32_t pd_offset = (virt_addr >> 22) & 0x3FF;
    uint32_t pt_offset = (virt_addr >> 12) & 0x3FF;

    // NOTE: in the bootstrap phase, everything in the kernel page
    //       tables are identity mapped. This also assumes the temp
    //       page mappings and the temp page mappings pt storage are
    //       in the same page table.
    uint32_t *pd = (void *) (get_cr3() & ~0xFFF);
    uint32_t *pt = (void *) (pd[pd_offset] & ~0xFFF);
    pt[pt_offset] = ((uintptr_t) pt) | 0x03;
    invlpg(virt_addr);
}

static void *create_temp_phys_addr_mapping_bsp(uintptr_t phys_addr, uint32_t *count) {
    assert(*count < 2);

    uint32_t virt_addr = (uintptr_t) __temp_page_mappings + *count * PAGE_SIZE;
    uint32_t pt_offset = (virt_addr >> 12) & 0x3FF;

    bsp_temp_page_mapping_pt[pt_offset] = phys_addr | 0x03;
    invlpg(virt_addr);

    *count += 1;

    return (void *) virt_addr;
}

static void free_temp_phys_addr_mapping_bsp(void *addr, uint32_t *count) {
    assert(*count > 0 && *count <= 2);

    *count -= 1;

    assert(addr = __temp_page_mappings + *count * PAGE_SIZE);
}

void *create_temp_phys_addr_mapping(uintptr_t phys_addr) {
    uintptr_t offset = phys_addr % PAGE_SIZE;
    phys_addr &= ~0xFFF;

    // NOTE: there's only one processor active, and interrupts are always disabled,
    //       and the vm allocation isn't possible yet.
    if (!bsp_enabled()) {
        return create_temp_phys_addr_mapping_bsp(phys_addr, &bsp_temp_page_mapping_count) + offset;
    }

    uint32_t irq_save = disable_interrupts_save();
    struct processor *processor = get_current_processor();
    if (processor->id == 0) {
        uint32_t *alloc_count = &processor->arch_processor.temp_page_alloc_count;
        if (*alloc_count == 0) {
            processor->arch_processor.temp_page_irq_save = irq_save;
        }
        return create_temp_phys_addr_mapping_bsp(phys_addr, alloc_count) + offset;
    }

    assert(false);
}

void free_temp_phys_addr_mapping(void *ptr) {
    ptr = (void *) ((uintptr_t) ptr & ~0xFFF);

    if (!bsp_enabled()) {
        free_temp_phys_addr_mapping_bsp(ptr, &bsp_temp_page_mapping_count);
        return;
    }

    struct processor *processor = get_current_processor();
    if (processor->id == 0) {
        uint32_t *alloc_count = &processor->arch_processor.temp_page_alloc_count;
        free_temp_phys_addr_mapping_bsp(ptr, alloc_count);
        if (*alloc_count == 0) {
            interrupts_restore(processor->arch_processor.temp_page_irq_save);
        }
        return;
    }

    assert(false);
}
