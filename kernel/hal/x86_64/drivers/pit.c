#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>

#include <kernel/hal/hw_device.h>
#include <kernel/hal/isa_driver.h>
#include <kernel/hal/output.h>
#include <kernel/hal/processor.h>
#include <kernel/hal/x86_64/drivers/pic.h>
#include <kernel/hal/x86_64/drivers/pit.h>
#include <kernel/irqs/handlers.h>
#include <kernel/proc/profile.h>
#include <kernel/proc/task.h>
#include <kernel/time/clock.h>
#include <kernel/util/init.h>

static void (*sched_callback)(struct task_state *) = NULL;
static unsigned int sched_count = 0;
static unsigned int sched_count_to = 0;

static void (*callback)(void) = NULL;
static unsigned int count = 0;
static unsigned int count_to = 0;

static struct timespec time_per_clock_tick;

extern struct task initial_kernel_task;
extern uint64_t idle_ticks;
extern uint64_t user_ticks;
extern uint64_t kernel_ticks;

void handle_pit_interrupt(struct irq_context *context) {
    struct task *current = get_current_task();
    if (current == get_idle_task()) {
        idle_ticks++;
    } else if (current->in_kernel) {
        current->process->rusage_self.ru_stime =
            time_add_timeval(current->process->rusage_self.ru_stime, timeval_from_time(time_per_clock_tick));
        kernel_ticks++;
    } else {
        current->process->rusage_self.ru_utime =
            time_add_timeval(current->process->rusage_self.ru_utime, timeval_from_time(time_per_clock_tick));
        user_ticks++;
    }
    // Check for NULL b/c kernel tasks don't have a clock
    if (current->task_clock) {
        time_inc_clock(current->task_clock, 1000000L, current->in_kernel);
        time_inc_clock(current->process->process_clock, 1000000L, current->in_kernel);
    }

    if (atomic_load(&current->process->should_profile)) {
        // To seriously support profiling multiple threads, the buffer and lock should be per-thread and not per-process.
        spin_lock(&current->process->profile_buffer_lock);
        // Make sure not to write into a stale buffer.
        if (current->process->profile_buffer) {
            proc_record_profile_stack(context->task_state);
        }
        spin_unlock(&current->process->profile_buffer_lock);
    }

    // NOTE: only call the timer callback on one CPU (its used for the monotonic and realtime clocks)
    if (callback != NULL && get_current_processor()->id == 0) {
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

    time_per_clock_tick.tv_sec = rate / 1000;
    time_per_clock_tick.tv_nsec = rate * 1000000;
}

static struct irq_handler pit_handler = { .handler = &handle_pit_interrupt, .flags = IRQ_HANDLER_EXTERNAL | IRQ_HANDLER_ALL_CPUS };

static void detect_pit(struct hw_device *parent) {
    struct hw_device *device = create_hw_device("PIT", parent, hw_device_id_isa(), NULL);
    device->status = HW_STATUS_ACTIVE;
    pit_set_rate(1);
    register_irq_handler(&pit_handler, PIT_IRQ_LINE + EXTERNAL_IRQ_OFFSET);
}

static struct isa_driver pit_driver = {
    .name = "x86 PIT",
    .detect_devices = detect_pit,
};

static void init_pit(void) {
    register_isa_driver(&pit_driver);
}
INIT_FUNCTION(init_pit, driver);
