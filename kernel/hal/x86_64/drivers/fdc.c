#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <kernel/arch/x86_64/asm_utils.h>
#include <kernel/hal/x86_64/drivers/fdc.h>
#include <kernel/hal/x86_64/drivers/pic.h>

static volatile bool has_been_interrupted = false;

#define wait_for_interrupt()      \
    has_been_interrupted = false; \
    while (!has_been_interrupted) \
        ;

static void handle_fdc_interrupt() {
    has_been_interrupted = true;
}

void init_fdc() {
    register_irq_line_handler(&handle_fdc_interrupt, FDC_IRQ_LINE, true);
}