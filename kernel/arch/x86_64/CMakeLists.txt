

set(KERNEL_ARCH_CFLAGS -mno-red-zone -mcmodel=large -mno-mmx -mno-sse -mno-sse2 PARENT_SCOPE)
set(KERNEL_ARCH_LDFLAGS -T ${CMAKE_CURRENT_SOURCE_DIR}/link.ld PARENT_SCOPE)

set(KERNEL_ARCHDIR arch/x86_64)
set(KERNEL_HALDIR hal/x86_64)

set(KERNEL_ARCH_SOURCES
    ${KERNEL_ARCHDIR}/irqs/asm_handlers.S
    ${KERNEL_ARCHDIR}/irqs/handlers.c
    ${KERNEL_ARCHDIR}/irqs/sys_calls.c
    ${KERNEL_ARCHDIR}/mem/page.c
    ${KERNEL_ARCHDIR}/mem/arch_vm_allocator.c
    ${KERNEL_ARCHDIR}/proc/run_task.S
    ${KERNEL_ARCHDIR}/proc/arch_task.c
    ${KERNEL_ARCHDIR}/proc/arch_process.c
    ${KERNEL_ARCHDIR}/sched/arch_task_sched.c
    ${KERNEL_ARCHDIR}/sched/sys_sched_run_next.S
    ${KERNEL_HALDIR}/drivers/ata.c
    ${KERNEL_HALDIR}/drivers/bga.c
    ${KERNEL_HALDIR}/drivers/cmos.c
    ${KERNEL_HALDIR}/drivers/e1000.c
    ${KERNEL_HALDIR}/drivers/fdc.c
    ${KERNEL_HALDIR}/drivers/keyboard.c
    ${KERNEL_HALDIR}/drivers/mouse.c
    ${KERNEL_HALDIR}/drivers/pci.c
    ${KERNEL_HALDIR}/drivers/pic_handler.S
    ${KERNEL_HALDIR}/drivers/pic.c
    ${KERNEL_HALDIR}/drivers/pit_handler.S
    ${KERNEL_HALDIR}/drivers/pit.c
    ${KERNEL_HALDIR}/drivers/serial.c
    ${KERNEL_HALDIR}/drivers/vga.c
    ${KERNEL_HALDIR}/gdt.c
    ${KERNEL_HALDIR}/hal.c
    ${KERNEL_HALDIR}/idt.c
    ${KERNEL_HALDIR}/irqs.c
    ${KERNEL_HALDIR}/output.c
    ${KERNEL_HALDIR}/timer.c
    PARENT_SCOPE
)