#ifndef _KERNEL_HAL_PROCESSOR_H
#define _KERNEL_HAL_PROCESSOR_H 1

#include <stdbool.h>
#include <stdint.h>
#include <kernel/hal/arch.h>

#include HAL_ARCH_SPECIFIC(processor.h)

struct vm_region;

struct processor {
    struct processor *self;
    struct processor *next;

    struct vm_region *kernel_stack;

    int id;
    bool enabled;

    struct arch_processor arch_processor;
};

struct processor *create_processor();
struct processor *get_processor_list(void);
struct processor *get_bsp(void);
void add_processor(struct processor *processor);
int processor_count(void);

void init_bsp(struct processor *processor);

#endif /* _KERNEL_HAL_HAL_H */
