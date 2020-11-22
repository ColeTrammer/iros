#ifndef _KERNEL_HAL_X86_64_DRIVERS_LOCAL_APIC_H
#define _KERNEL_HAL_X86_64_DRIVERS_LOCAL_APIC_H 1

#define APIC_MSR_ENABLE_LOCAL 0x800

#define LOCAL_APIC_IRQ_OFFSET 48
#define LOCAL_APIC_IPI_IRQ    48
#define LOCAL_APIC_PANIC_IRQ  49
#define LOCAL_APIC_TIMER_IRQ  50
#define LOCAL_APIC_IRQ_END    63

union local_apic_icr {
    struct {
        uint64_t vector : 8;
#define LOCAL_APIC_ICR_MESSAGE_TYPE_FIXED        0
#define LOCAL_APIC_ICR_MESSAGE_TYPE_LOW_PRIO     1
#define LOCAL_APIC_ICR_MESSAGE_TYPE_SMI          2
#define LOCAL_APIC_ICR_MESSAGE_TYPE_NMI          4
#define LOCAL_APIC_ICR_MESSAGE_TYPE_INIT         5
#define LOCAL_APIC_ICR_MESSAGE_TYPE_SIPI         6
#define LOCAL_APIC_ICR_MESSAGE_TYPE_EXTERNAL_IRQ 7
        uint64_t message_type : 3;
#define LOCAL_APIC_ICR_DESTINATION_MODE_PHYSICAL 0
#define LOCAL_APIC_ICR_DESTINATION_MODE_LOGICAL  1
        uint64_t destination_mode : 1;
        uint64_t delivery_status : 1;
        uint64_t reserved0 : 1;
#define LOCAL_APIC_ICR_LEVEL_DEASSERT 0
#define LOCAL_APIC_ICR_LEVEL_ASSERT   1
        uint64_t level : 1;
#define LOCAL_APIC_ICR_TRIGGER_MODE_EDGE  0
#define LOCAL_APIC_ICR_TRIGGER_MODE_LEVEL 1
        uint64_t trigger_mode : 1;
        uint64_t remote_read_address : 2;
#define LOCAL_APIC_ICR_DESTINATION_SHORTHAND_TARGETED        0
#define LOCAL_APIC_ICR_DESTINATION_SHORTHAND_SELF            1
#define LOCAL_APIC_ICR_DESTINATION_SHORTHAND_ALL             2
#define LOCAL_APIC_ICR_DESTINATION_SHORTHAND_ALL_EXCEPT_SELF 3
        uint64_t destination_shorthand : 2;
        uint64_t resserved2 : 12;

        uint64_t reserved3 : 24;
        uint64_t destination : 4;
        uint64_t reserved4 : 4;
    };
    uint64_t raw_value;
};

_Static_assert(sizeof(union local_apic_icr) == sizeof(uint64_t));

struct local_apic_timer_lvt {
    union {
        struct {
            uint32_t vector : 8;
            uint32_t reserved0 : 4;
            uint32_t delivery_status : 1;
            uint32_t reserved1 : 3;
            uint32_t mask : 1;
#define LOCAL_APIC_TIMER_MODE_ONE_SHOT     0b00
#define LOCAL_APIC_TIMER_MODE_PERIODIC     0b01
#define LOCAL_APIC_TIMER_MODE_TSC_DEADLINE 0b10
            uint32_t mode : 2;
        };
        uint32_t raw_value;
    };
};

struct local_apic {
#define APIC_LOCAL_STRUCT_ENTRY(name) \
    union {                           \
        uint32_t name;                \
        uint8_t __padding_##name[16]; \
    }

#define APIC_LOCAL_STRUCT_ENTRY_TYPE(name, t) \
    union {                                   \
        t name;                               \
        uint8_t __padding_##name[16];         \
    }

    APIC_LOCAL_STRUCT_ENTRY(reserved) reserved0[2];
    APIC_LOCAL_STRUCT_ENTRY(lapic_id_register);
    APIC_LOCAL_STRUCT_ENTRY(lapic_version_register);
    APIC_LOCAL_STRUCT_ENTRY(reserved) reserved1[4];
    APIC_LOCAL_STRUCT_ENTRY(task_priority_register);
    APIC_LOCAL_STRUCT_ENTRY(arbitration_priority_register);
    APIC_LOCAL_STRUCT_ENTRY(processor_priority_register);
    APIC_LOCAL_STRUCT_ENTRY(eoi_register);
    APIC_LOCAL_STRUCT_ENTRY(remote_read_register);
    APIC_LOCAL_STRUCT_ENTRY(logical_destination_register);
    APIC_LOCAL_STRUCT_ENTRY(destination_format_register);
    APIC_LOCAL_STRUCT_ENTRY(spurious_interrupt_vector_register);
    APIC_LOCAL_STRUCT_ENTRY(value) in_service_register[8];
    APIC_LOCAL_STRUCT_ENTRY(value) trigger_mode_register[8];
    APIC_LOCAL_STRUCT_ENTRY(value) interrupt_request_register[8];
    APIC_LOCAL_STRUCT_ENTRY(error_status_register);
    APIC_LOCAL_STRUCT_ENTRY(reserved) reserved2[6];
    APIC_LOCAL_STRUCT_ENTRY(lvt_corrected_machine_check_interrupt_register);
    APIC_LOCAL_STRUCT_ENTRY(value) interrupt_command_register[2];
    APIC_LOCAL_STRUCT_ENTRY_TYPE(lvt_timer_register, struct local_apic_timer_lvt);
    APIC_LOCAL_STRUCT_ENTRY(lvt_thermal_sensor_register);
    APIC_LOCAL_STRUCT_ENTRY(lvt_performance_monitoring_counters_register);
    APIC_LOCAL_STRUCT_ENTRY(value) lvt_lint_register[2];
    APIC_LOCAL_STRUCT_ENTRY(lvt_error_register);
    APIC_LOCAL_STRUCT_ENTRY(initial_count_register);
    APIC_LOCAL_STRUCT_ENTRY(current_count_register);
    APIC_LOCAL_STRUCT_ENTRY(reserved) reserved3[4];
    APIC_LOCAL_STRUCT_ENTRY(divide_configuration_register);
    APIC_LOCAL_STRUCT_ENTRY(reserved) reserved4[1];

#undef APIC_LOCAL_STRUCT_ENTRY
#undef APIC_LOCAL_STRUCT_ENTRY_TYPE
} __attribute__((packed));

_Static_assert(sizeof(struct local_apic) == 0x400);

void local_apic_send_eoi(void);
void local_apic_start_aps(void);
void local_apic_broadcast_ipi(int irq);
void local_apic_send_ipi(uint8_t apic_id, int irq);

void init_local_apic(void);
void init_local_apic_irq_handlers(void);

#endif /* _KERNEL_HAL_X86_64_DRIVERS_LOCAL_APIC_H */
