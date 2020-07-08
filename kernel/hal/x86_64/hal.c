#include <sys/types.h>
#include <kernel/hal/devices.h>
#include <kernel/hal/hal.h>
#include <kernel/hal/irqs.h>
#include <kernel/hal/output.h>
#include <kernel/hal/processor.h>
#include <kernel/mem/page.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/proc/task.h>

#include <kernel/arch/x86_64/asm_utils.h>
#include <kernel/hal/x86_64/acpi.h>
#include <kernel/hal/x86_64/drivers/ata.h>
#include <kernel/hal/x86_64/drivers/cmos.h>
#include <kernel/hal/x86_64/drivers/fdc.h>
#include <kernel/hal/x86_64/drivers/keyboard.h>
#include <kernel/hal/x86_64/drivers/local_apic.h>
#include <kernel/hal/x86_64/drivers/mouse.h>
#include <kernel/hal/x86_64/drivers/pci.h>
#include <kernel/hal/x86_64/drivers/pic.h>
#include <kernel/hal/x86_64/drivers/pit.h>
#include <kernel/hal/x86_64/drivers/serial.h>
#include <kernel/hal/x86_64/drivers/vga.h>
#include <kernel/hal/x86_64/drivers/vmware_back_door.h>
#include <kernel/hal/x86_64/gdt.h>

static bool supports_rdrand;

bool cpu_supports_rdrand(void) {
    return supports_rdrand;
}

static void detect_cpu_features(void) {
    uint32_t a, b, c, d;
    cpuid(CPUID_FEATURES, &a, &b, &c, &d);
    supports_rdrand = !!(c & CPUID_ECX_RDRAND);
}

void init_hal(void) {
    detect_cpu_features();

    // Set the IDT so that CPU execeptions can be handled
    init_irqs();

    // Set up the serial port debug output
    init_serial_ports();

    // Read cmos now so that kernel time is initialized ASAP
    init_cmos();

    debug_log("Finished Initializing HAL\n");
}

void init_cpus(void) {
    // Parse the acpi tables now that dynamic memory allocation is available.
    init_acpi();

#ifdef KERNEL_USE_PIC
    init_pic();
#else
    init_local_apic();
#endif /* KERNEL_USE_PIC */
}

void init_drivers(void) {
    init_keyboard();
    init_mouse();
    init_vmware_back_door();
    init_fdc();
    init_ata();
    init_serial_port_device(SERIAL_COM1_PORT, 0);
    init_pit();

    if (!kernel_use_graphics()) {
        init_vga_device();
    }

    init_pci();

    init_virtual_devices();
    debug_log("Finished Initializing Drivers\n");
}

void init_smp(void) {
    local_apic_start_aps();
}

extern struct task initial_kernel_task;

void init_bsp(struct processor *processor) {
    processor->enabled = true;
    processor->kernel_stack = vm_allocate_kernel_region(PAGE_SIZE);
    init_gdt(processor);

    set_msr(MSR_GS_BASE, 0);
    set_msr(MSR_KERNEL_GS_BASE, (uint64_t) processor);
    swapgs();

    init_idle_task(processor);
}

static bool use_graphics = true;

void kernel_disable_graphics(void) {
    use_graphics = false;
    debug_log("kernel graphics disabled\n");
}

bool kernel_use_graphics(void) {
    return use_graphics;
}
