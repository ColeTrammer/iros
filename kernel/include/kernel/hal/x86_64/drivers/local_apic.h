#ifndef _KERNEL_HAL_X86_64_DRIVERS_LOCAL_APIC_H
#define _KERNEL_HAL_X86_64_DRIVERS_LOCAL_APIC_H 1

#define APIC_MSR_ENABLE_LOCAL 0x800

union local_apic_icr {
    struct {
        uint64_t irq_number : 8;
#define LOCAL_APIC_ICR_DEST_MODE_NORMAL   0
#define LOCAL_APIC_ICR_DEST_MODE_LOW_PRIO 1
#define LOCAL_APIC_ICR_DEST_MODE_SMI      2
#define LOCAL_APIC_ICR_DEST_MODE_NMI      4
#define LOCAL_APIC_ICR_DEST_MODE_INIT     5
#define LOCAL_APIC_ICR_DEST_MODE_SIPI     6
        uint64_t destination_mode : 3;
        uint64_t destination_is_logical : 1;
        uint64_t delivery_status : 1;
        uint64_t reserved0 : 1;
#define LOCAL_APIC_ICR_NO_DE_ASSERT 1
#define LOCAL_APIC_ICR_DE_ASSERT    2
        uint64_t init_level_de_assert : 2;
        uint64_t reserved1 : 2;
#define LOCAL_APIC_ICR_DEST_TYPE_TARGETED        0
#define LOCAL_APIC_ICR_DEST_TYPE_SELF            1
#define LOCAL_APIC_ICR_DEST_TYPE_ALL             2
#define LOCAL_APIC_ICR_DEST_TYPE_ALL_EXCEPT_SELF 3
        uint64_t destination_type : 2;
        uint64_t resserved2 : 12;

        uint64_t reserved3 : 24;
        uint64_t target_apic_id : 4;
        uint64_t reserved4 : 4;
    };
    uint64_t raw_value;
};

_Static_assert(sizeof(union local_apic_icr) == sizeof(uint64_t));

struct local_apic {
#define APIC_LOCAL_STRUCT_ENTRY(name) \
    union {                           \
        uint32_t name;                \
        uint8_t __padding_##name[16]; \
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
    APIC_LOCAL_STRUCT_ENTRY(lvt_timer_register);
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
} __attribute__((packed));

_Static_assert(sizeof(struct local_apic) == 0x400);

void local_apic_send_eoi(void);
void local_apic_start_aps(void);

void init_local_apic(void);

#endif /* _KERNEL_HAL_X86_64_DRIVERS_LOCAL_APIC_H */
