#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>

#include <kernel/hal/output.h>
#include <kernel/hal/x86_64/drivers/pic.h>
#include <kernel/hal/x86_64/drivers/pit.h>
#include <kernel/irqs/handlers.h>
#include <kernel/proc/task.h>
#include <kernel/time/clock.h>

static void (*sched_callback)(struct task_state *) = NULL;
static unsigned int sched_count = 0;
static unsigned int sched_count_to = 0;

static void (*callback)(void) = NULL;
static unsigned int count = 0;
static unsigned int count_to = 0;

extern struct task initial_kernel_task;
extern uint64_t idle_ticks;
extern uint64_t user_ticks;
extern uint64_t kernel_ticks;

void handle_pit_interrupt(struct irq_context *context) {

    struct task *current = get_current_task();
    if (current == &initial_kernel_task) {
        idle_ticks++;
    } else if (current->in_kernel) {
        current->process->times.tms_stime++;
        kernel_ticks++;
    } else {
        current->process->times.tms_utime++;
        user_ticks++;
    }
    // Check for NULL b/c kernel tasks don't have a clock
    if (current->task_clock) {
        time_inc_clock(current->task_clock, 1000000L);
        // NOTE: we can't simply inc the process clock if we were doing SMP
        time_inc_clock(current->process->process_clock, 1000000L);
    }

    if (callback != NULL) {
        count++;
        if (count >= count_to) {
            count = 0;
            callback();
        }
    }

    context->irq_controller->ops->send_eoi(context->irq_controller, context->irq_num);
    if (sched_callback != NULL) {
        sched_count++;
        if (sched_count >= sched_count_to) {
            sched_count = 0;
            sched_callback(context->task_state);
        }
    }
}

void pit_set_sched_callback(void (*_sched_callback)(struct task_state *), unsigned int ms) {
    sched_callback = _sched_callback;
    sched_count_to = ms;
}

void pit_register_callback(void (*_callback)(void), unsigned int ms) {
    callback = _callback;
    count_to = ms;
}

void pit_set_rate(unsigned int rate) {
    PIT_SET_MODE(0, PIT_ACCESS_LOHI, PIT_MODE_SQUARE_WAVE);
    outb(PIT_CHANNEL_0, PIT_GET_DIVISOR(rate) & 0xFF);
    outb(PIT_CHANNEL_0, PIT_GET_DIVISOR(rate) >> 8);
}

static struct irq_handler pit_handler = { .handler = &handle_pit_interrupt, .flags = IRQ_HANDLER_EXTERNAL };

void init_pit() {
#ifdef USE_PIT
    pit_set_rate(1);
#else
    pit_set_rate(2);
#endif /* USE_PIT */
    register_irq_handler(&pit_handler, PIT_IRQ_LINE + EXTERNAL_IRQ_OFFSET);
}
