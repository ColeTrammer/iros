#include <stdatomic.h>
#include <string.h>

#include <kernel/hal/hw_timer.h>
#include <kernel/hal/processor.h>
#include <kernel/hal/x86_64/acpi.h>
#include <kernel/hal/x86_64/drivers/local_apic.h>
#include <kernel/hal/x86_64/drivers/pic.h>
#include <kernel/hal/x86_64/gdt.h>
#include <kernel/hal/x86_64/idt.h>
#include <kernel/irqs/handlers.h>
#include <kernel/mem/kernel_vm.h>
#include <kernel/mem/page.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/proc/task.h>
#include <kernel/sched/task_sched.h>
#include <kernel/time/clock.h>

void local_apic_send_eoi(void) {
    struct acpi_info *info = acpi_get_info();
    volatile struct local_apic *local_apic = create_phys_addr_mapping(info->local_apic_address);

    local_apic->eoi_register = 0;
}

static union local_apic_icr read_icr(volatile struct local_apic *local_apic) {
    return (union local_apic_icr) { .raw_value = ((uint64_t) local_apic->interrupt_command_register[1].value) << 32 |
                                                 (uint64_t) local_apic->interrupt_command_register[0].value };
}

static void write_icr(volatile struct local_apic *local_apic, union local_apic_icr command) {
    local_apic->interrupt_command_register[1].value = (command.raw_value >> 32) & 0xFFFFFFFFU;
    local_apic->interrupt_command_register[0].value = command.raw_value & 0xFFFFFFFFU;
}

void local_apic_broadcast_ipi(int vector) {
    struct acpi_info *info = acpi_get_info();
    volatile struct local_apic *local_apic = create_phys_addr_mapping(info->local_apic_address);

    union local_apic_icr command = { .raw_value = 0 };
    command.vector = vector;
    command.trigger_mode = LOCAL_APIC_ICR_TRIGGER_MODE_EDGE;
    command.destination_shorthand = LOCAL_APIC_ICR_DESTINATION_SHORTHAND_ALL_EXCEPT_SELF;

    while (read_icr(local_apic).delivery_status) {
        io_wait_us(200);
    }
    write_icr(local_apic, command);
}

void local_apic_send_ipi(uint8_t apic_id, int vector) {
    struct acpi_info *info = acpi_get_info();
    volatile struct local_apic *local_apic = create_phys_addr_mapping(info->local_apic_address);

    union local_apic_icr command = { .raw_value = 0 };
    command.vector = vector;
    command.trigger_mode = LOCAL_APIC_ICR_TRIGGER_MODE_EDGE;
    command.destination = apic_id;

    while (read_icr(local_apic).delivery_status) {
        io_wait_us(200);
    }
    write_icr(local_apic, command);
}

void init_ap(struct processor *processor) {
    atomic_store(&processor->enabled, true);
    arch_init_processor(processor);

    debug_log("~Processor %u successfully booted\n", processor->id);
    sched_run_next();
}

struct ap_trampoline {
    uint64_t cr3;
    uint64_t idt;
    uint64_t rsp;
    struct processor *processor;
} __attribute__((packed));

static void start_ap(volatile struct local_apic *local_apic, struct processor *processor) {
    struct vm_region *code_trampoline = vm_allocate_low_identity_map(0x8000, KERNEL_AP_TRAMPOLINE_END - KERNEL_AP_TRAMPOLINE_START);
    assert(get_phys_addr(0x8000) == 0x8000);
    assert(code_trampoline->end > 0x8000 + KERNEL_AP_TRAMPOLINE_END - KERNEL_AP_TRAMPOLINE_START);

    struct vm_region *ap_stack = processor->kernel_stack = vm_allocate_kernel_region(PAGE_SIZE);
    memcpy((void *) code_trampoline->start, (void *) KERNEL_AP_TRAMPOLINE_START, KERNEL_AP_TRAMPOLINE_END - KERNEL_AP_TRAMPOLINE_START);

    struct ap_trampoline *trampoline =
        (void *) (code_trampoline->start + KERNEL_AP_TRAMPOLINE_END - KERNEL_AP_TRAMPOLINE_START - sizeof(struct ap_trampoline));
    trampoline->cr3 = get_cr3();
    trampoline->idt = (uintptr_t) get_idt_descriptor();
    trampoline->rsp = ap_stack->end;
    trampoline->processor = processor;

    union local_apic_icr command = { .raw_value = 0 };
    command.message_type = LOCAL_APIC_ICR_MESSAGE_TYPE_INIT;
    command.destination_mode = LOCAL_APIC_ICR_DESTINATION_MODE_PHYSICAL;
    command.level = LOCAL_APIC_ICR_LEVEL_ASSERT;
    command.trigger_mode = LOCAL_APIC_ICR_TRIGGER_MODE_EDGE;
    command.destination = processor->arch_processor.local_apic_id;

    debug_log("Sending INIT to CPU: [ %u, %#.16lX ]\n", processor->arch_processor.local_apic_id, trampoline->cr3);
    write_icr(local_apic, command);

    io_wait_us(10 * 1000);

    command.vector = 8;
    command.message_type = LOCAL_APIC_ICR_MESSAGE_TYPE_SIPI;

    bool loaded = false;
    for (int i = 0; i < 2; i++) {
        debug_log("Sending SIPI to CPU: [ %u ]\n", processor->id);
        write_icr(local_apic, command);
        io_wait_us(200);

        if (atomic_load(&processor->enabled)) {
            loaded = true;
            break;
        }
    }

    if (!loaded) {
        for (int i = 0; i < 1000000; i++) {
            if (atomic_load(&processor->enabled)) {
                break;
            }
            io_wait();
        }
    }

    vm_free_low_identity_map(code_trampoline);
}

void local_apic_start_aps(void) {
    struct acpi_info *info = acpi_get_info();
    volatile struct local_apic *local_apic = create_phys_addr_mapping(info->local_apic_address);

    struct processor *processor = get_processor_list();
    while (processor) {
        // NOTE: processor id 0 should be the BSP
        if (processor->id != 0) {
            start_ap(local_apic, processor);
        }

        processor = processor->next;
    }
}

static struct hw_timer *lapic_timer;
static struct wait_queue lapic_timer_wq = WAIT_QUEUE_INITIALIZER(lapic_timer_wq);

static void lapic_do_wakeup(struct hw_timer_channel *channel, struct irq_context *context) {
    (void) context;

    channel->timer->ops->disable_channel(channel->timer, hw_timer_channel_index(channel));
    wake_up_all(&lapic_timer_wq);
}

bool handle_lapic_timer_interrupt(struct irq_context *context) {
    struct hw_timer_channel *channel = context->closure;
    if (get_current_processor()->id != hw_timer_channel_index(channel)) {
        return false;
    }

    context->irq_controller->ops->send_eoi(context->irq_controller, context->irq_num);

    if (channel->callback) {
        channel->callback(channel, context);
    }
    return true;
}

static void lapic_timer_setup_interval_timer(struct hw_timer *self, int channel_index, long frequency, hw_timer_callback_t callback) {
    struct timespec interval = (struct timespec) { .tv_nsec = 1000000000 / frequency };
    unsigned long ticks = time_divide(interval, self->base_frequency);
    assert(ticks <= UINT32_MAX);

    struct hw_timer_channel *channel = &self->channels[channel_index];
    assert(!channel->valid);

    init_hw_timer_channel(channel, handle_lapic_timer_interrupt, IRQ_HANDLER_EXTERNAL | IRQ_HANDLER_NO_EOI | IRQ_HANDLER_SHARED, self,
                          HW_TIMER_INTERVAL, frequency, callback);
    register_irq_handler(&channel->irq_handler, LOCAL_APIC_TIMER_IRQ);

    struct acpi_info *info = acpi_get_info();
    volatile struct local_apic *local_apic = create_phys_addr_mapping(info->local_apic_address);

    local_apic->lvt_timer_register = (struct local_apic_timer_lvt) {
        .mode = LOCAL_APIC_TIMER_MODE_ONE_SHOT,
        .vector = LOCAL_APIC_TIMER_IRQ,
    };
    local_apic->divide_configuration_register = 0b1011;
    local_apic->initial_count_register = ticks;
}

static void lapic_timer_setup_one_shot_timer(struct hw_timer *self, int channel_index, struct timespec delay,
                                             hw_timer_callback_t callback) {
    unsigned long ticks = time_divide(delay, self->base_frequency);
    assert(ticks <= UINT32_MAX);

    struct hw_timer_channel *channel = &self->channels[channel_index];
    assert(!channel->valid);

    init_hw_timer_channel(channel, handle_lapic_timer_interrupt, IRQ_HANDLER_EXTERNAL | IRQ_HANDLER_NO_EOI, self, HW_TIMER_SINGLE_SHOT, 0,
                          callback);
    register_irq_handler(&channel->irq_handler, LOCAL_APIC_TIMER_IRQ);

    struct acpi_info *info = acpi_get_info();
    volatile struct local_apic *local_apic = create_phys_addr_mapping(info->local_apic_address);

    local_apic->lvt_timer_register = (struct local_apic_timer_lvt) {
        .mode = LOCAL_APIC_TIMER_MODE_ONE_SHOT,
        .vector = LOCAL_APIC_TIMER_IRQ,
    };
    local_apic->divide_configuration_register = 0b1011;
    local_apic->initial_count_register = ticks;
}

static void lapic_timer_disable_channel(struct hw_timer *self, int channel_index) {
    struct hw_timer_channel *channel = &self->channels[channel_index];

    struct acpi_info *info = acpi_get_info();
    volatile struct local_apic *local_apic = create_phys_addr_mapping(info->local_apic_address);

    uint64_t save = disable_interrupts_save();
    unregister_irq_handler(&channel->irq_handler, LOCAL_APIC_TIMER_IRQ);

    local_apic->lvt_timer_register.mask = 1;

    interrupts_restore(save);
    destroy_hw_timer_channel(channel);
}

static void lapic_timer_calibrate(struct hw_timer *self, struct hw_timer *reference) {
    struct acpi_info *info = acpi_get_info();
    volatile struct local_apic *local_apic = create_phys_addr_mapping(info->local_apic_address);

    local_apic->divide_configuration_register = 0b1011;

    reference->ops->setup_one_shot_timer(reference, 0, (struct timespec) { .tv_nsec = 10 * 1000000 }, lapic_do_wakeup);

    local_apic->initial_count_register = 0xFFFFFFFFU;
    wait_simple(get_current_task(), &lapic_timer_wq);

    local_apic->lvt_timer_register.mask = 1;
    uint32_t elapsed_ticks = 0xFFFFFFFFU - local_apic->current_count_register;

    long frequency = 100 * elapsed_ticks;
    self->base_frequency = frequency;
    self->max_resolution = (struct timespec) { .tv_nsec = 1000000000 / frequency };
}

static struct hw_timer_ops lapic_timer_ops = {
    .setup_interval_timer = lapic_timer_setup_interval_timer,
    .setup_one_shot_timer = lapic_timer_setup_one_shot_timer,
    .disable_channel = lapic_timer_disable_channel,
    .calibrate = lapic_timer_calibrate,
};

void init_local_apic(void) {
    struct acpi_info *info = acpi_get_info();
    set_msr(MSR_LOCAL_APIC_BASE, info->local_apic_address | APIC_MSR_ENABLE_LOCAL);

    volatile struct local_apic *local_apic = create_phys_addr_mapping(info->local_apic_address);
    local_apic->spurious_interrupt_vector_register = 0x1FF;

    if (!lapic_timer) {
        debug_log("!!\n");
        lapic_timer = create_hw_timer("APIC Timer", root_hw_device(), hw_device_id_isa(),
                                      HW_TIMER_SINGLE_SHOT | HW_TIMER_INTERVAL | HW_TIMER_PER_CPU | HW_TIMER_NEEDS_CALIBRATION, 0,
                                      &lapic_timer_ops, processor_count());
        lapic_timer->hw_device.status = HW_STATUS_ACTIVE;
        register_hw_timer(lapic_timer);
    }
}

static bool handle_ipi(struct irq_context *context) {
    (void) context;

    handle_processor_messages();
    return true;
}

static bool handle_panic(struct irq_context *context) {
    context->irq_controller->ops->send_eoi(context->irq_controller, context->irq_num);

    disable_interrupts();
    for (;;) {}
    return true;
}

static struct irq_handler ipi_handler = {
    .handler = handle_ipi,
    .flags = IRQ_HANDLER_EXTERNAL | IRQ_HANDLER_ALL_CPUS,
};
static struct irq_handler panic_handler = {
    .handler = handle_panic,
    .flags = IRQ_HANDLER_EXTERNAL | IRQ_HANDLER_ALL_CPUS | IRQ_HANDLER_NO_EOI,
};

static bool lapic_is_valid_irq(struct irq_controller *self __attribute__((unused)), int irq_num __attribute__((unused))) {
    return true;
}

static void lapic_send_eoi(struct irq_controller *self __attribute__((unused)), int irq_num __attribute__((unused))) {
    local_apic_send_eoi();
}

static void lapic_set_irq_enabled(struct irq_controller *self __attribute__((unused)), int irq_num __attribute__((unused)),
                                  bool enabled __attribute__((unused))) {}

static void lapic_map_irq(struct irq_controller *self __attribute__((unused)), int irq_num __attribute__((unused)),
                          int flags __attribute__((unused))) {}

static struct irq_controller_ops local_apic_controller_ops = {
    .is_valid_irq = &lapic_is_valid_irq,
    .send_eoi = &lapic_send_eoi,
    .set_irq_enabled = &lapic_set_irq_enabled,
    .map_irq = &lapic_map_irq,
};

static struct irq_controller local_apic_controller = {
    .irq_start = LOCAL_APIC_IRQ_OFFSET,
    .irq_end = LOCAL_APIC_IRQ_END,
    .ops = &local_apic_controller_ops,
};

void init_local_apic_irq_handlers(void) {
    register_irq_controller(&local_apic_controller);
    register_irq_handler(&ipi_handler, LOCAL_APIC_IPI_IRQ);
    register_irq_handler(&panic_handler, LOCAL_APIC_PANIC_IRQ);
}
