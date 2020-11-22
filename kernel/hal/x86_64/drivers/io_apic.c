#include <stdlib.h>

#include <kernel/hal/x86_64/acpi.h>
#include <kernel/hal/x86_64/drivers/io_apic.h>
#include <kernel/hal/x86_64/drivers/local_apic.h>
#include <kernel/irqs/handlers.h>
#include <kernel/mem/vm_allocator.h>

static struct io_apic *io_apics;

static uint32_t read_io_apic_register(struct io_apic *io_apic, uint32_t reg) {
    io_apic->memory->register_select = reg;
    return io_apic->memory->register_value;
}

static void write_io_apic_register(struct io_apic *io_apic, uint32_t reg, uint32_t value) {
    io_apic->memory->register_select = reg;
    io_apic->memory->register_value = value;
}

static union io_apic_entry read_io_apic_entry(struct io_apic *io_apic, uint8_t irq) {
    uint32_t low_bits = read_io_apic_register(io_apic, IO_APIC_REDIRECT_ENTIRES_BASE + 2 * irq);
    uint32_t high_bits = read_io_apic_register(io_apic, IO_APIC_REDIRECT_ENTIRES_BASE + 2 * irq + 1);
    return (union io_apic_entry) { .raw_value = ((uint64_t) high_bits << 32UL) | low_bits };
}

static void write_io_apic_entry(struct io_apic *io_apic, uint8_t irq, union io_apic_entry entry) {
    write_io_apic_register(io_apic, IO_APIC_REDIRECT_ENTIRES_BASE + 2 * irq, entry.raw_value & 0xFFFFFFFFU);
    write_io_apic_register(io_apic, IO_APIC_REDIRECT_ENTIRES_BASE + 2 * irq + 1, (entry.raw_value >> 32U) & 0xFFFFFFFFU);
}

static bool io_apic_is_valid_irq(struct irq_controller *controller, int irq) {
    (void) controller;
    (void) irq;

    // Unlike the PIC, I don't believe there is any way to test if the requested IRQ is
    // actually requested. It is up to individual devices to determine if their IRQ was
    // valid.
    return true;
}

static void io_apic_send_eoi(struct irq_controller *controller, int irq_num) {
    (void) controller;
    (void) irq_num;

    // EOI requests go to the local APIC.
    local_apic_send_eoi();
}

static void io_apic_set_irq_enabled(struct irq_controller *controller, int irq_num, bool enabled) {
    struct io_apic *io_apic = controller->private;
    int num_irqs = controller->irq_end - controller->irq_start + 1;
    for (int i = 0; i < num_irqs; i++) {
        union io_apic_entry entry = read_io_apic_entry(io_apic, i);
        if (entry.value.generated_irq == irq_num) {
            if (!enabled) {
                entry.raw_value = 0;
                entry.value.generated_irq = 0xFF;
            }
            entry.value.mask_bit = !enabled;
            write_io_apic_entry(io_apic, i, entry);
            return;
        }
    }

    debug_log("IO APIC does not have an entry for IRQ: [ %d ]\n", irq_num);
}

static void io_apic_apply_interrupt_source_override(struct io_apic *io_apic, uint8_t mode, uint8_t mapped_irq, uint32_t irq_source,
                                                    uint16_t flags, uint8_t destination) {
    union io_apic_entry entry = read_io_apic_entry(io_apic, irq_source);
    entry.value.delivery_mode = mode;
    entry.value.generated_irq = mapped_irq + IO_APIC_IRQ_OFFSET;
    entry.value.pin_polarity = !!(flags & 0b0010);
    entry.value.trigger_mode = !!(flags & 0b1000);
    entry.value.destination = destination;
    write_io_apic_entry(io_apic, irq_source, entry);
}

static void io_apic_map_irq(struct irq_controller *controller, int irq_num, int flags) {
    struct io_apic *io_apic = controller->private;
    uint8_t mode = (flags & IRQ_HANDLER_REQUEST_NMI) ? LOCAL_APIC_ICR_MESSAGE_TYPE_NMI : LOCAL_APIC_ICR_MESSAGE_TYPE_FIXED;

    struct acpi_info *info = acpi_get_info();
    for (size_t i = 0; i < info->interrupt_source_overrides_length; i++) {
        // FIXME: does the interrupt source override `bus' field matter at all?
        if ((int) info->interrupt_source_override[i].irq_source == irq_num - IO_APIC_IRQ_OFFSET) {
            io_apic_apply_interrupt_source_override(io_apic, mode, info->interrupt_source_override[i].irq_source,
                                                    info->interrupt_source_override[i].global_system_interrupt,
                                                    info->interrupt_source_override[i].flags, flags & IRQ_HANDLER_ALL_CPUS ? 0xFF : 0);
            return;
        }
    }

    // Default mapping is 1 to 1, no flags set
    io_apic_apply_interrupt_source_override(io_apic, mode, irq_num - IO_APIC_IRQ_OFFSET, irq_num - IO_APIC_IRQ_OFFSET, 0,
                                            flags & IRQ_HANDLER_ALL_CPUS ? 0xFF : 0);
}

static struct irq_controller_ops io_apic_ops = { .is_valid_irq = &io_apic_is_valid_irq,
                                                 .send_eoi = &io_apic_send_eoi,
                                                 .set_irq_enabled = &io_apic_set_irq_enabled,
                                                 .map_irq = &io_apic_map_irq };

void create_io_apic(uint8_t acpi_id, uintptr_t base_phys_addr, uint32_t irq_base) {
    struct io_apic *io_apic = malloc(sizeof(struct io_apic));
    io_apic->memory = create_phys_addr_mapping(base_phys_addr);
    io_apic->id = acpi_id;

    uint32_t id_register = read_io_apic_register(io_apic, IO_APIC_REGISTER_ID);
    uint8_t id = (id_register & 0x00F000000U) >> 24U;
    if (acpi_id != id) {
        debug_log("IO APIC ids don't match: [ %u, %u ]\n", acpi_id, id);
        free(io_apic);
        return;
    }

    uint32_t version_register = read_io_apic_register(io_apic, IO_APIC_REGISTER_VERSION);
    uint8_t max_irq_entry = (version_register & 0x00FF0000U) >> 16U;
    debug_log("IO APIC irqs: [ %u, %u ]\n", id, max_irq_entry + 1);

    // Initialize all entries to map to an ignored IRQ, be disabled, and to send IRQs to the processor with ID 0
    for (uint8_t irq = 0; irq <= max_irq_entry; irq++) {
        union io_apic_entry entry = read_io_apic_entry(io_apic, irq);
        entry.value.generated_irq = 0xFF;
        entry.value.delivery_mode = 0;
        entry.value.destination_mode = 0;
        entry.value.pin_polarity = 0;
        entry.value.trigger_mode = 0;
        entry.value.mask_bit = 1;
        // FIXME: the boot CPU might not have an id of 0?
        entry.value.destination = 0;
        write_io_apic_entry(io_apic, irq, entry);
    }

    io_apic->next = io_apics;
    io_apics = io_apic;

    register_irq_controller(
        create_irq_controller(IO_APIC_IRQ_OFFSET + irq_base, IO_APIC_IRQ_OFFSET + irq_base + max_irq_entry, &io_apic_ops, io_apic));
}
