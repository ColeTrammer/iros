#include <string.h>
#include <stdint.h>

#include "gdt.h"

static struct gdt_entry gdt[GDT_ENTRIES];
static struct gdt_descriptor gdt_descriptor;

static struct tss tss;

static uint8_t reserved_stack[4 * 4096];

void init_gdt() {
    memset(&gdt, 0, GDT_ENTRIES * sizeof(struct gdt_entry));
    gdt[CS_OFFSET].type = 0b10011010;
    gdt[CS_OFFSET].flags = 0b0010;

    gdt[DATA_OFFSET].type = 0b10010010;
    
    gdt[USER_CODE_OFFSET].type = 0b11111010;
    gdt[USER_CODE_OFFSET].flags = 0b0010;

    gdt[USER_DATA_OFFSET].type = 0b11110010;

    struct gdt_tss_entry *tss_entry = (struct gdt_tss_entry*) (gdt + TSS_OFFSET);
    tss_entry->type = TSS_TYPE;
    tss_entry->limit_low = sizeof(struct tss);
    tss_entry->base_low      = ((uintptr_t) &tss)  & 0x000000000000FFFF;
    tss_entry->base_low_mid  = (((uintptr_t) &tss) & 0x0000000000FF0000) >> 16;
    tss_entry->base_high_mid = (((uintptr_t) &tss) & 0x00000000FF000000) >> 24;
    tss_entry->base_high     = (((uintptr_t) &tss) & 0xFFFFFFFF00000000) >> 32;

    gdt_descriptor.limit = GDT_ENTRIES * sizeof(struct gdt_entry) - 1;
    gdt_descriptor.gdt = gdt;
    load_gdt(gdt_descriptor);

    memset(&tss, 0, sizeof(struct tss));
    tss.rsp0 = (uintptr_t) (reserved_stack + 4 * 4096);
    tss.io_map_base = sizeof(struct tss);
    load_tr(TSS_SELECTOR);
}