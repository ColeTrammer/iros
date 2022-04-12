

set(KERNEL_ARCH_CFLAGS -mno-mmx -mno-sse -mno-sse2 PARENT_SCOPE)
set(KERNEL_ARCH_LDFLAGS -T ${CMAKE_CURRENT_SOURCE_DIR}/link.ld PARENT_SCOPE)

set(KERNEL_ARCHDIR arch/i686)
set(KERNEL_HALDIR hal/i686)

set(KERNEL_ARCH_SOURCES
    arch/x86/sched/arch_task_sched.c
    arch/x86/proc/fpu.c
    ${KERNEL_ARCHDIR}/sched/__kernel_yield.S
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
    hal/x86/processor.c
    ${KERNEL_HALDIR}/output.c
    PARENT_SCOPE
)
