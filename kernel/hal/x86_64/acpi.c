#include <string.h>

#include <kernel/hal/x86_64/acpi.h>
#include <kernel/mem/page.h>
#include <kernel/mem/vm_allocator.h>

uintptr_t find_rsdp(void) {
    char *bios_data_area = create_phys_addr_mapping(0);
    uintptr_t extended_bios_data_area_addr = *((uint16_t *) (&bios_data_area[0x040E])) << 4;
    uint16_t extended_bios_data_area_size = *((uint16_t *) (&bios_data_area[0x413]));
    debug_log("EBDA address: [ %#.16lX, %#.8X ]\n", extended_bios_data_area_addr, extended_bios_data_area_size);

    char *extended_bios_data_area = create_phys_addr_mapping(extended_bios_data_area_addr & ~(PAGE_SIZE - 1));
    extended_bios_data_area += extended_bios_data_area_addr % PAGE_SIZE;
    debug_log("EBDA mapping: [ %p ]\n", extended_bios_data_area);

    char *rsdp_string = "RSD PTR ";
    for (size_t i = 0; i < extended_bios_data_area_size; i += 16) {
        if (memcmp(extended_bios_data_area + i, rsdp_string, 8) == 0) {
            return (uintptr_t)(extended_bios_data_area + i) & 0xFFFFFFFF;
        }
    }

    char *bios_rom = create_phys_addr_mapping(0x000E0000);
    size_t bios_rom_size = 0x00100000 - 0x000E0000;
    for (size_t i = 0; i < bios_rom_size; i += 16) {
        if (memcmp(bios_rom + i, rsdp_string, 8) == 0) {
            return (uintptr_t)(bios_rom + i) & 0xFFFFFFFF;
        }
    }

    return -1;
}

void init_acpi(void) {
    uintptr_t rsdp_addr = find_rsdp();
    if (rsdp_addr == (uintptr_t) -1) {
        debug_log("Count not find RSDP; ACPI cannot be enabled.\n");
        return;
    }

    debug_log("Found RSDP: [ %#.16lX ]\n", rsdp_addr);
}
