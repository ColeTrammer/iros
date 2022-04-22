

set(KERNEL_ARCH_CFLAGS -mno-mmx -mno-sse -mno-sse2 -mno-red-zone PARENT_SCOPE)
set(KERNEL_ARCH_LDFLAGS -T ${CMAKE_CURRENT_SOURCE_DIR}/link.ld PARENT_SCOPE)

set(KERNEL_ARCHDIR arch/i686)
set(KERNEL_HALDIR hal/i686)

set(KERNEL_ARCH_SOURCES
    arch/x86/irqs/handlers.c
    arch/x86/proc/arch_process.c
    arch/x86/proc/arch_profile.c
    arch/x86/proc/fpu.c
    arch/x86/sched/arch_task_sched.c
    ${KERNEL_ARCHDIR}/irqs/asm_handlers.S
    ${KERNEL_ARCHDIR}/mem/arch_page.c
    ${KERNEL_ARCHDIR}/mem/arch_vm_allocator.c
    ${KERNEL_ARCHDIR}/proc/arch_task.c
    ${KERNEL_ARCHDIR}/proc/run_task.S
    ${KERNEL_ARCHDIR}/sched/__kernel_yield.S
    ${KERNEL_ARCHDIR}/ap_boot.S
    ${KERNEL_ARCHDIR}/boot.S
    hal/x86/drivers/ata_drive.c
    hal/x86/drivers/bga.c
    hal/x86/drivers/ide_channel.c
    hal/x86/drivers/ide_controller.c
    hal/x86/drivers/io_apic.c
    hal/x86/drivers/keyboard.c
    hal/x86/drivers/local_apic.c
    hal/x86/drivers/mouse.c
    hal/x86/drivers/pic.c
    hal/x86/drivers/pit.c
    hal/x86/drivers/ps2.c
    hal/x86/drivers/rtc.c
    hal/x86/drivers/serial.c
    hal/x86/drivers/vga.c
    hal/x86/drivers/vmware_back_door.c
    hal/x86/acpi.c
    hal/x86/arch_pci.c
    hal/x86/hal.c
    hal/x86/irqs.c
    hal/x86/processor.c
    ${KERNEL_HALDIR}/idt.c
    ${KERNEL_HALDIR}/gdt.c
    ${KERNEL_HALDIR}/output.c
    ${KERNEL_HALDIR}/processor.c
    PARENT_SCOPE
)
