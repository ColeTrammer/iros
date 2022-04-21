#include <inttypes.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/hal/processor.h>
#include <kernel/hal/x86/acpi.h>
#include <kernel/hal/x86/drivers/io_apic.h>
#include <kernel/mem/page.h>
#include <kernel/mem/vm_allocator.h>

static bool validate_table(void *addr, size_t length) {
    uint8_t check = 0;
    uint8_t *a = addr;
    for (size_t i = 0; i < length; i++) {
        check += a[i];
    }

    return !check;
}

uintptr_t find_rsdp() {
    char *bios_data_area = create_temp_phys_addr_mapping(0);
    uintptr_t extended_bios_data_area_addr = *((uint16_t *) (&bios_data_area[0x040E])) << 4;
    uint16_t extended_bios_data_area_size = *((uint16_t *) (&bios_data_area[0x413]));
    free_temp_phys_addr_mapping(bios_data_area);

    char *extended_bios_data_area = (void *) extended_bios_data_area_addr;
    debug_log("EBDA address: [ %p, %#.8X ]\n", extended_bios_data_area, extended_bios_data_area_size);

    char *rsdp_string = "RSD PTR ";
    for (size_t i = 0; i < extended_bios_data_area_size; i += 16) {
        if (memcmp(extended_bios_data_area + i, rsdp_string, 8) == 0) {
            return (uintptr_t) (extended_bios_data_area + i) & 0xFFFFFFFF;
        }
    }

    char *bios_rom = (void *) 0x000E0000;
    size_t bios_rom_size = 0x00100000 - 0x000E0000;
    for (size_t i = 0; i < bios_rom_size; i += 16) {
        if (memcmp(bios_rom + i, rsdp_string, 8) == 0) {
            return (uintptr_t) (bios_rom + i) & 0xFFFFFFFF;
        }
    }

    return -1;
}

uintptr_t find_table(struct acpi_rsdt *rsdt, char *table_name) {
    for (size_t i = 0; i < (rsdt->header.length - sizeof(rsdt->header)) / sizeof(rsdt->table_pointers[0]); i++) {
        struct acpi_table_header *table = create_temp_phys_addr_mapping(rsdt->table_pointers[i]);
        if (memcmp(table->signature, table_name, 4) == 0) {
            free_temp_phys_addr_mapping(table);
            debug_log("Found table: [ %s, %p ]\n", table_name, table);
            return rsdt->table_pointers[i];
        }
        free_temp_phys_addr_mapping(table);
    }

    return -1;
}

static struct acpi_info s_acpi_info;

struct acpi_info *acpi_get_info(void) {
    return &s_acpi_info;
}

bool init_acpi(void) {
    bool found_acpi_tables = false;
    struct acpi_rsdp *rsdp = NULL;
    struct acpi_rsdt *rsdt = NULL;
    struct acpi_madt *madt = NULL;
    struct vm_region *id_region = vm_allocate_low_identity_map(PAGE_SIZE, 0x100000 - PAGE_SIZE);

    uintptr_t rsdp_addr = find_rsdp();
    if (rsdp_addr == (uintptr_t) -1) {
        debug_log("Can not find RSDP; ACPI cannot be enabled.\n");
        goto cleanup;
    }
    vm_free_low_identity_map(id_region);
    id_region = NULL;

    debug_log("Found RSDP: [ %p ]\n", (void *) rsdp_addr);
    rsdp = create_temp_phys_addr_mapping(rsdp_addr);
    if (!validate_table(rsdp, offsetof(struct acpi_rsdp, length))) {
        debug_log("RSDP is invalid\n");
        goto cleanup;
    }

    char *oem_id_string = strndup(rsdp->oem_id, sizeof(rsdp->oem_id));
    debug_log("OEM ID: [ %s ]\n", oem_id_string);
    free(oem_id_string);

    debug_log("ACPI revision: [ %u ]\n", rsdp->revision);
    assert(rsdp->revision == 0);

    uintptr_t rsdt_addr = rsdp->rsdt_address;
    free_temp_phys_addr_mapping(rsdp);
    rsdp = NULL;

    rsdt = create_temp_phys_addr_mapping(rsdt_addr);
    debug_log("Found RSDT: [ %p ]\n", rsdt);
    if (!validate_table(rsdt, rsdt->header.length)) {
        debug_log("RSDT is invalid");
        goto cleanup;
    }

    uintptr_t madt_addr = find_table(rsdt, "APIC");
    if (madt_addr == (uintptr_t) -1) {
        debug_log("Can not find MADT");
        goto cleanup;
    }
    free_temp_phys_addr_mapping(rsdt);
    rsdt = NULL;

    madt = create_temp_phys_addr_mapping(madt_addr);
    if (!validate_table(madt, madt->header.length)) {
        debug_log("MADT is invalid");
        goto cleanup;
    }

    debug_log("Local APIC Address: [ %#.8X ]\n", madt->local_apic_addr);
    s_acpi_info.local_apic_address = madt->local_apic_addr;
    s_acpi_info.local_apic_vm_region = vm_allocate_physically_mapped_kernel_region(s_acpi_info.local_apic_address, PAGE_SIZE);

    debug_log("MADT flags: [ %#.8X ]\n", madt->flags);
    debug_log("MADT length: [ %u ]\n", madt->header.length);

    for (size_t offset = 0; offset < madt->header.length - sizeof(struct acpi_madt);) {
        struct acpi_madt_entry *entry = (struct acpi_madt_entry *) &madt->entries[offset];
        offset += entry->length;

        switch (entry->type) {
            case ACPI_MADT_TYPE_LOCAL_APIC: {
                debug_log("Local APIC: [ %u, %u, %#.8X ]\n", entry->local_apic.acpi_processor_id, entry->local_apic.apic_id,
                          entry->local_apic.flags);

                struct processor *processor = create_processor(entry->local_apic.acpi_processor_id);
                processor->arch_processor.acpi_id = entry->local_apic.acpi_processor_id;
                processor->arch_processor.local_apic_id = entry->local_apic.apic_id;
                add_processor(processor);
                break;
            }
            case ACPI_MADT_TYPE_IO_APIC:
                debug_log("IO APIC: [ %u, %#.8X, %#.8X ]\n", entry->io_apic.io_apic_id, entry->io_apic.io_apic_address,
                          entry->io_apic.global_system_interrupt_base);
#ifndef KERNEL_USE_PIC
                create_io_apic(entry->io_apic.io_apic_id, entry->io_apic.io_apic_address, entry->io_apic.global_system_interrupt_base);
#endif /* KERNEL_USE_PIC */
                break;
            case ACPI_MADT_TYPE_INTERRUPT_SOURCE_OVERRIDE:
                debug_log("Interrupt Source Overrides: [ %u, %u, %u, %#.4X ]\n", entry->interrupt_source_override.bus_source,
                          entry->interrupt_source_override.irq_source, entry->interrupt_source_override.global_system_interrupt,
                          entry->interrupt_source_override.flags);
                assert(s_acpi_info.interrupt_source_overrides_length < 16);
                s_acpi_info.interrupt_source_override[s_acpi_info.interrupt_source_overrides_length++] = entry->interrupt_source_override;
                break;
            case ACPI_MADT_TYPE_NON_MASKABLE_INTERRUPTS:
                debug_log("Non Maskable Interrupts: [ %u, %u, %u ]\n", entry->non_maskable_interrupts.acpi_processor_id,
                          entry->non_maskable_interrupts.flags, entry->non_maskable_interrupts.local_int_number);
                break;
            case ACPI_MADT_TYPE_LOCAL_APIC_ADDRESS_OVERRIDE:
                debug_log("Local APIC Address Override: [ %.16" PRIX64 " ]\n", entry->local_apic_address_override.local_apic_phys_addr);
                s_acpi_info.local_apic_address = entry->local_apic_address_override.local_apic_phys_addr;
                break;
        }
    }

    found_acpi_tables = true;

cleanup:
    if (id_region) {
        vm_free_low_identity_map(id_region);
    }
    if (rsdp) {
        free_temp_phys_addr_mapping(rsdp);
    }
    if (rsdt) {
        free_temp_phys_addr_mapping(rsdt);
    }
    if (madt) {
        free_temp_phys_addr_mapping(madt);
    }
    return found_acpi_tables;
}
