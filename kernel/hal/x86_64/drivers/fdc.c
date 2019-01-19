#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "pic.h"
#include "io.h"
#include "fdc.h"

static volatile bool has_been_interrupted = false;

#define wait_for_interrupt() \
    has_been_interrupted = false; \
    while (!has_been_interrupted);

void handle_fdc_interrupt() {
    has_been_interrupted = true;
}

void init_fdc() {
    register_irq_line_handler(&handle_fdc_interrupt, FDC_IRQ_LINE);
}