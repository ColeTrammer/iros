#include <kernel/hal/hal.h>
#include <kernel/hal/irqs.h>
#include <kernel/hal/output.h>

#include <kernel/hal/x86_64/gdt.h>
#include <kernel/hal/x86_64/drivers/pic.h>
#include <kernel/hal/x86_64/drivers/pit.h>
#include <kernel/hal/x86_64/drivers/keyboard.h>
#include <kernel/hal/x86_64/drivers/fdc.h>
#include <kernel/hal/x86_64/drivers/serial.h>

void init_hal() {
    init_irqs();
    
    init_pic();
    init_serial_ports();
    init_output();

    init_gdt();

    debug_log("Finished Initializing HAL\n");
}

void init_drivers() {
    init_keyboard();
    init_fdc();
    init_pit();

    debug_log("Finished Initializing Drivers\n");
}

void enable_interrupts() {
    asm ( "sti" );

    debug_log("Interrupts Enabled\n");
}