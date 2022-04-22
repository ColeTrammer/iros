#include <kernel/hal/processor.h>
#include <kernel/mem/page.h>
#include <kernel/mem/vm_allocator.h>

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

void vm_bootstrap_temp_page_mapping_processor(struct processor *processor) {
    struct vm_region *mapping_region = vm_allocate_kernel_vm(0x2000);

    uint32_t virt_addr = (uintptr_t) mapping_region->start;
    uint32_t pd_offset = (virt_addr >> 22) & 0x3FF;

    // NOTE: the current processor has already bootstrapped its get_temp_phys_addr_mapping(),
    //       its valid to use it here. Additionally, all page directories are already filled
    //       in by the kernel, so its not possible for pd[pd_offset] to not be present.
    uint32_t *pd = create_temp_phys_addr_mapping(get_cr3() & ~0xFFF);
    uintptr_t pt_phys_addr = pd[pd_offset] & ~0xFFF;
    free_temp_phys_addr_mapping(pd);

    struct vm_region *page_table_region = vm_allocate_physically_mapped_kernel_region(pt_phys_addr, PAGE_SIZE);
    processor->arch_processor.temp_page_vm = mapping_region;
    processor->arch_processor.temp_page_page_table_vm = page_table_region;
}

static void *do_create_temp_phys_addr_mapping(uintptr_t phys_addr, uint32_t *count, void *temp_page_mappings,
                                              uint32_t *temp_page_pt_mapping) {
    assert(*count < 2);

    uint32_t virt_addr = (uintptr_t) temp_page_mappings + *count * PAGE_SIZE;
    uint32_t pt_offset = (virt_addr >> 12) & 0x3FF;

    temp_page_pt_mapping[pt_offset] = phys_addr | 0x03;
    invlpg(virt_addr);

    *count += 1;

    return (void *) virt_addr;
}

static void do_free_temp_phys_addr_mapping(void *addr, uint32_t *count, void *temp_page_mappings) {
    assert(*count > 0 && *count <= 2);

    *count -= 1;

    assert(addr == temp_page_mappings + *count * PAGE_SIZE);
}

void *create_temp_phys_addr_mapping(uintptr_t phys_addr) {
    uintptr_t offset = phys_addr % PAGE_SIZE;
    phys_addr &= ~0xFFF;

    // NOTE: there's only one processor active, and interrupts are always disabled,
    //       and the vm allocation isn't possible yet.
    if (!bsp_enabled()) {
        return do_create_temp_phys_addr_mapping(phys_addr, &bsp_temp_page_mapping_count, __temp_page_mappings, bsp_temp_page_mapping_pt) +
               offset;
    }

    uint32_t irq_save = disable_interrupts_save();
    struct processor *processor = get_current_processor();
    uint32_t *alloc_count = &processor->arch_processor.temp_page_alloc_count;
    if (*alloc_count == 0) {
        processor->arch_processor.temp_page_irq_save = irq_save;
    }

    if (processor->id == 0) {
        return do_create_temp_phys_addr_mapping(phys_addr, alloc_count, __temp_page_mappings, bsp_temp_page_mapping_pt) + offset;
    }
    return do_create_temp_phys_addr_mapping(phys_addr, alloc_count, (void *) processor->arch_processor.temp_page_vm->start,
                                            (void *) processor->arch_processor.temp_page_page_table_vm->start) +
           offset;
}

void free_temp_phys_addr_mapping(void *ptr) {
    ptr = (void *) ((uintptr_t) ptr & ~0xFFF);

    if (!bsp_enabled()) {
        do_free_temp_phys_addr_mapping(ptr, &bsp_temp_page_mapping_count, __temp_page_mappings);
        return;
    }

    struct processor *processor = get_current_processor();
    uint32_t *alloc_count = &processor->arch_processor.temp_page_alloc_count;
    if (processor->id == 0) {
        do_free_temp_phys_addr_mapping(ptr, alloc_count, __temp_page_mappings);
    } else {
        do_free_temp_phys_addr_mapping(ptr, alloc_count, (void *) processor->arch_processor.temp_page_vm->start);
    }
    if (*alloc_count == 0) {
        interrupts_restore(processor->arch_processor.temp_page_irq_save);
    }
}
