#ifndef _KERNEL_HAL_X86_DRIVERS_IO_APIC_H
#define _KERNEL_HAL_X86_DRIVERS_IO_APIC_H 1

#include <stdint.h>

#define IO_APIC_IRQ_OFFSET 32

#define IO_APIC_REGISTER_ID           0x00
#define IO_APIC_REGISTER_VERSION      0x01
#define IO_APIC_REGISTER_ARB          0x02
#define IO_APIC_REDIRECT_ENTIRES_BASE 0x10

union io_apic_entry {
    struct {
        uint64_t generated_irq : 8;
        uint64_t delivery_mode : 3;
        uint64_t destination_mode : 1;
        uint64_t delivery_status : 1;
        uint64_t pin_polarity : 1;
        uint64_t remove_irr : 1;
        uint64_t trigger_mode : 1;
        uint64_t mask_bit : 1;
        uint64_t reserved : 39;
        uint64_t destination : 8;
    } value __attribute__((packed));
    uint64_t raw_value;
} __attribute__((packed));

_Static_assert(sizeof(union io_apic_entry) == sizeof(uint64_t));

struct io_apic_registers {
    union {
        uint32_t register_select;
        uint8_t __padding0[16];
    };
    union {
        uint32_t register_value;
        uint8_t __padding1[16];
    };
} __attribute__((packed));

struct io_apic {
    struct io_apic *next;
    struct vm_region *memory_mapped_region;
    volatile struct io_apic_registers *memory;
    uint8_t id;
};

void create_io_apic(uint8_t id, uintptr_t base_phys_memory, uint32_t irq_offset);

#endif /* _KERNEL_HAL_X86_DRIVERS_IO_APIC_H */
