#include <hal/hal.h>
#include <hal/irqs.h>

#include "gdt.h"

void init_hal() {
    init_gdt();
    init_irqs();
}