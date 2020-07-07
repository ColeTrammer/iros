#ifndef _KERNEL_HAL_PROCESSOR_H
#define _KERNEL_HAL_PROCESSOR_H 1

#include <stdbool.h>
#include <stdint.h>
#include <kernel/hal/arch.h>

#include HAL_ARCH_SPECIFIC(processor.h)

struct processor {
    struct processor *next;
    uint8_t id;
    bool enabled;

    struct arch_processor arch_processor;
};

struct processor *create_processor(uint8_t id);
struct processor *get_processor_list(void);
void add_processor(struct processor *processor);

#endif /* _KERNEL_HAL_HAL_H */
