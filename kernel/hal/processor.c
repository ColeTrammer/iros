#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/hal/output.h>
#include <kernel/hal/processor.h>
#include <kernel/sched/task_sched.h>

// #define SCHED_DEBUG

static struct processor *processor_list;
static int num_processors;
static bool s_smp_enabled;

void set_smp_enabled() {
    s_smp_enabled = true;
}

bool smp_enabled(void) {
    return s_smp_enabled;
}

struct processor *create_processor() {
    struct processor *processor = malloc(sizeof(struct processor));
    processor->self = processor;
    processor->next = NULL;
    processor->ipi_messages_head = processor->ipi_messages_tail = NULL;
    init_spinlock(&processor->ipi_messages_lock);
    processor->current_task = processor->sched_list_start = processor->sched_list_end = NULL;
    processor->idle_task = NULL;
    processor->kernel_stack = NULL;
    processor->id = num_processors++;
    processor->enabled = false;
    return processor;
}

struct processor *get_processor_list(void) {
    return processor_list;
}

void add_processor(struct processor *processor) {
    // NOTE: this should be called during boot when there is no potential for alternate CPUs to be running.
    processor->next = processor_list;
    processor_list = processor;

    debug_log("Processor detected: [ %p, %u ]\n", processor, processor->id);

    if (processor->id == 0) {
        init_bsp(processor);
    }
}

int processor_count(void) {
    return num_processors;
}

void broadcast_panic(void) {
    if (processor_count() <= 1) {
        return;
    }

    struct processor *current = get_current_processor();
    if (!current || !smp_enabled()) {
        return;
    }

    arch_broadcast_panic();
}

struct processor_ipi_message *ipi_message_pool_head;
spinlock_t ipi_message_pool_lock = SPINLOCK_INITIALIZER;

void init_processor_ipi_messages(void) {
    if (processor_count() <= 1) {
        return;
    }

    ipi_message_pool_head =
        malloc(100 * (sizeof(struct processor_ipi_message) + sizeof(struct processor_ipi_message *) * processor_count()));
    for (size_t i = 0; i < 100; i++) {
        struct processor_ipi_message *current =
            (struct processor_ipi_message *) (((uint8_t *) ipi_message_pool_head) +
                                              i * (sizeof(struct processor_ipi_message) +
                                                   sizeof(struct processor_ipi_message *) * processor_count()));
        struct processor_ipi_message *next =
            (struct processor_ipi_message *) (((uint8_t *) ipi_message_pool_head) +
                                              (i + 1) * (sizeof(struct processor_ipi_message) +
                                                         sizeof(struct processor_ipi_message *) * processor_count()));
        current->type = PROCESSOR_IPI_FREED;
        current->next_free = i == 99 ? NULL : next;
    }
}

struct processor_ipi_message *allocate_processor_ipi_message(void) {
    spin_lock(&ipi_message_pool_lock);
    struct processor_ipi_message *message = ipi_message_pool_head;
    if (message) {
        ipi_message_pool_head = message->next_free;
    }
    spin_unlock(&ipi_message_pool_lock);

    if (!message) {
        return NULL;
    }

    assert(message->type == PROCESSOR_IPI_FREED);
    memset(message->next, 0, sizeof(struct processor_ipi_message *) * processor_count());
    message->ref_count = 1;

    return message;
}

static void free_processor_ipi_message(struct processor_ipi_message *message) {
    message->type = PROCESSOR_IPI_FREED;

    spin_lock(&ipi_message_pool_lock);
    message->next_free = ipi_message_pool_head;
    ipi_message_pool_head = message;
    spin_unlock(&ipi_message_pool_lock);
}

void bump_processor_ipi_message(struct processor_ipi_message *message) {
    atomic_fetch_add(&message->ref_count, 1);
}

void drop_processor_ipi_message(struct processor_ipi_message *message) {
    int fetched_ref_count = atomic_fetch_sub(&message->ref_count, 1);
    if (fetched_ref_count == 1) {
        free_processor_ipi_message(message);
    }
}

void enqueue_processor_ipi_message(struct processor *processor, struct processor_ipi_message *message) {
    assert(message->type != PROCESSOR_IPI_FREED);
    spin_lock(&processor->ipi_messages_lock);
    if (processor->ipi_messages_head == NULL) {
        processor->ipi_messages_head = processor->ipi_messages_tail = message;
    } else {
        processor->ipi_messages_tail->next[processor->id] = message;
    }
    processor->ipi_messages_tail = message;
    spin_unlock(&processor->ipi_messages_lock);
}

void schedule_task_on_processor(struct task *task, struct processor *processor) {
#ifdef SCHED_DEBUG
    debug_log("Scheduling task on processor: [ %d:%d, %d ]\n", task->tid, task->process->pid, processor->id);
#endif /* SCHED_DEBUG */

    if (processor == get_current_processor()) {
        local_sched_add_task(task);
        return;
    }

    struct processor_ipi_message *message = allocate_processor_ipi_message();
    assert(message);
    message->type = PROCESSOR_IPI_SCHEDULE_TASK;
    message->schedule_task.task = task;

    enqueue_processor_ipi_message(processor, message);
    arch_send_ipi(processor);

    // NOTE: the other processor will clean up the message
}

void broadcast_flush_tlb(uintptr_t base, size_t pages) {
    if (processor_count() <= 1) {
        return;
    }

    uint64_t save = disable_interrupts_save();

    struct processor *current = get_current_processor();
    if (!current || !smp_enabled()) {
        return;
    }

    struct processor_ipi_message *message = allocate_processor_ipi_message();
    assert(message);
    message->type = PROCESSOR_IPI_FLUSH_TLB;
    message->flush_tlb.base = base;
    message->flush_tlb.pages = pages;

    bool sent_message = false;
    struct processor *processor = get_processor_list();
    while (processor) {
        if (processor != current && processor->enabled) {
            bump_processor_ipi_message(message);
            enqueue_processor_ipi_message(processor, message);
            sent_message = true;
        }

        processor = processor->next;
    }

    if (sent_message) {
        arch_broadcast_ipi();
    }

    while (atomic_load(&message->ref_count) > 1) {
        handle_processor_messages();
        cpu_relax();
    }
    free_processor_ipi_message(message);

    interrupts_restore(save);
}
