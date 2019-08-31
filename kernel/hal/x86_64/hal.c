#include <kernel/hal/hal.h>
#include <kernel/hal/irqs.h>

#include <kernel/hal/x86_64/gdt.h>
#include <kernel/hal/x86_64/drivers/pic.h>
#include <kernel/hal/x86_64/drivers/keyboard.h>
#include <kernel/hal/x86_64/drivers/fdc.h>

void init_hal() {
    init_gdt();
    init_irqs();
}

void init_drivers() {
    init_pic();
    init_keyboard();
    init_fdc();
}

void enable_interrupts() {
    asm ( "sti" );
}