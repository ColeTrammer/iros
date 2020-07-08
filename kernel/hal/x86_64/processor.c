#include <kernel/arch/x86_64/asm_utils.h>
#include <kernel/hal/processor.h>
#include <kernel/hal/x86_64/drivers/local_apic.h>
#include <kernel/mem/page.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/proc/task.h>

void arch_init_processor(struct processor *processor) {
    init_gdt(processor);
    init_local_apic();
    set_msr(MSR_GS_BASE, 0);
    set_msr(MSR_KERNEL_GS_BASE, (uintptr_t) processor);
    swapgs();
    init_idle_task(processor);
}

void init_bsp(struct processor *processor) {
    processor->enabled = true;
    processor->kernel_stack = vm_allocate_kernel_region(PAGE_SIZE);

    arch_init_processor(processor);
}
