#include <hal/hal.h>
#include <hal/irqs.h>

#include "gdt.h"

#include "drivers/pic.h"
#include "drivers/fdc.h"

void init_hal() {
    init_gdt();
    init_irqs();
}

void init_drivers() {
    init_pic();
    init_fdc();
}

void enable_interrupts() {
    asm ( "sti" );
}