#ifndef _KERNEL_HAL_PROCESSOR_H
#define _KERNEL_HAL_PROCESSOR_H 1

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <kernel/hal/arch.h>

#include HAL_ARCH_SPECIFIC(processor.h)

struct task;
struct vm_region;

struct processor {
    struct processor *self;
    struct processor *next;

    struct task *idle_task;
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

void arch_init_processor(struct processor *processor);
void init_bsp(struct processor *processor);

static inline struct task *get_idle_task(void) {
    return get_current_processor()->idle_task;
}

#endif /* _KERNEL_HAL_HAL_H */
