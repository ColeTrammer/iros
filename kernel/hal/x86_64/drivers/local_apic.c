#include <stdatomic.h>
#include <string.h>

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

void init_local_apic(void) {
    struct acpi_info *info = acpi_get_info();
    set_msr(MSR_LOCAL_APIC_BASE, info->local_apic_address | APIC_MSR_ENABLE_LOCAL);

    volatile struct local_apic *local_apic = create_phys_addr_mapping(info->local_apic_address);
    local_apic->spurious_interrupt_vector_register = 0x1FF;
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
    .flags = IRQ_HANDLER_EXTERNAL | IRQ_HANDLER_ALL_CPUS,
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
    .is_valid_irq = &lapic_is_valid_irq, .send_eoi = &lapic_send_eoi, .set_irq_enabled = &lapic_set_irq_enabled, .map_irq = &lapic_map_irq
};

static struct irq_controller local_apic_controller = { .irq_start = LOCAL_APIC_IRQ_OFFSET,
                                                       .irq_end = LOCAL_APIC_IRQ_END,
                                                       .ops = &local_apic_controller_ops };

void init_local_apic_irq_handlers(void) {
    register_irq_controller(&local_apic_controller);
    register_irq_handler(&ipi_handler, LOCAL_APIC_IPI_IRQ);
    register_irq_handler(&panic_handler, LOCAL_APIC_PANIC_IRQ);
}
