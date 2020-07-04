#ifndef _KERNEL_HAL_X86_64_ACPI_H
#define _KERNEL_HAL_X86_64_ACPI_H 1

#include <stdint.h>

struct acpi_rsdp {
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;

    /* Extended Fields (revision > 0) */
    uint32_t rsdt_address;
    uint32_t length;
    uint64_t xsdr_address;
    uint8_t extended_checksum;
    uint8_t reserved[3];
} __attribute__((packed));

struct acpi_table_header {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} __attribute__((packed));

struct acpi_rsdt {
    struct acpi_table_header header;
    union {
        uint32_t table_pointers[0];
        uint64_t extended_table_pointers[0];
    };
} __attribute__((packed));

#define ACPI_MADT_TYPE_LOCAL_APIC                  0
#define ACPI_MADT_TYPE_IO_APIC                     1
#define ACPI_MADT_TYPE_INTERRUPT_SOURCE_OVERRIDE   2
#define ACPI_MADT_TYPE_NON_MASKABLE_INTERRUPTS     4
#define ACPI_MADT_TYPE_LOCAL_APIC_ADDRESS_OVERRIDE 5

struct acpi_madt_entry {
    uint8_t type;
    uint8_t length;
    union {
        struct {
            uint8_t acpi_processor_id;
            uint8_t apic_id;
            uint32_t flags;
        } __attribute__((packed)) local_apic;
        struct {
            uint8_t io_apic_id;
            uint8_t reserved;
            uint32_t io_apic_address;
            uint32_t global_system_interrupt_base;
        } __attribute__((packed)) io_apic;
        struct {
            uint8_t bus_source;
            uint8_t irq_source;
            uint32_t global_system_interrupt;
            uint16_t flags;
        } __attribute__((packed)) interrupt_source_override;
        struct {
            uint8_t acpi_processor_id;
            uint16_t flags;
            uint8_t local_int_number;
        } __attribute__((packed)) non_maskable_interrupts;
        struct {
            uint16_t reserved;
            uint64_t local_apic_phys_addr;
        } __attribute__((packed)) local_apic_address_override;
    };
} __attribute__((packed));

struct acpi_madt {
    struct acpi_table_header header;
    uint32_t local_apic_addr;
    uint32_t flags;
    uint8_t entries[0];
} __attribute__((packed));

struct acpi_info {
    uintptr_t local_apic_address;
};

void init_acpi(void);
struct acpi_info *acpi_get_info(void);

#endif /* _KERNEL_HAL_X86_64_ACPI_H */
