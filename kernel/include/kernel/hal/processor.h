#ifndef _KERNEL_HAL_PROCESSOR_H
#define _KERNEL_HAL_PROCESSOR_H 1

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <kernel/hal/arch.h>
#include <kernel/util/list.h>
#include <kernel/util/spinlock.h>

#include HAL_ARCH_SPECIFIC(processor.h)

struct task;
struct vm_region;

enum processor_ipi_message_type {
    PROCESSOR_IPI_FREED,
    PROCESSOR_IPI_FLUSH_TLB,
    PROCESSOR_IPI_SCHEDULE_TASK,
};

struct processor_ipi_message {
    int ref_count;
    enum processor_ipi_message_type type;
    union {
        struct processor_ipi_message *next_free;
        struct {
            uintptr_t base;
            size_t pages;
        } flush_tlb;
        struct {
            struct task *task;
        } schedule_task;
    };

    struct processor_ipi_message *next[0];
};
struct processor {
    struct processor *self;
    struct processor *next;

    struct arch_processor arch_processor;

    struct task *idle_task;
    struct vm_region *kernel_stack;

    struct processor_ipi_message *ipi_messages_head;
    struct processor_ipi_message *ipi_messages_tail;
    spinlock_t ipi_messages_lock;

    struct task *current_task;
    struct list_node sched_list;
    spinlock_t sched_lock;
    bool sched_idle;

    int preemption_disabled_count;

    int id;
    bool enabled;
};

struct processor *create_processor();
struct processor *get_processor_list(void);
struct processor *get_bsp(void);
void add_processor(struct processor *processor);
int processor_count(void);

void arch_init_processor(struct processor *processor);
void init_bsp(struct processor *processor);
bool bsp_enabled(void);

void set_smp_enabled();
bool smp_enabled(void);

void broadcast_panic(void);
void arch_broadcast_panic(void);
void arch_broadcast_ipi(void);
void arch_send_ipi(struct processor *processor);
void broadcast_flush_tlb(uintptr_t base, size_t pages);
void schedule_task_on_processor(struct task *task, struct processor *processor);

void init_processor_ipi_messages(void);
struct processor_ipi_message *allocate_processor_ipi_message(void);
void bump_processor_ipi_message(struct processor_ipi_message *message);
void drop_processor_ipi_message(struct processor_ipi_message *message);
void enqueue_processor_ipi_message(struct processor *processor, struct processor_ipi_message *message);
void handle_processor_messages(void);

void sched_maybe_yield(void);

static inline struct task *get_idle_task(void) {
    return get_current_processor()->idle_task;
}

static inline struct task *get_current_task(void) {
    return get_current_processor()->current_task;
}

static inline void set_current_task(struct task *task) {
    // FIXME: assert interrupts are disabled.
    get_current_processor()->current_task = task;
    assert(get_current_task() == task);
}

static inline void disable_preemption(void) {
    struct processor *processor = get_current_processor();
    processor->preemption_disabled_count++;
}

static inline void __enable_preemption(void) {
    struct processor *processor = get_current_processor();
    assert(processor->preemption_disabled_count != 0);
    processor->preemption_disabled_count--;
}

static inline void enable_preemption(void) {
    __enable_preemption();
    sched_maybe_yield();
}
#endif /* _KERNEL_HAL_HAL_H */
