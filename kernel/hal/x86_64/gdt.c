#include <string.h>

#include "gdt.h"

static struct gdt_entry gdt[GDT_ENTRIES];
static struct gdt_descriptor gdt_descriptor;

void init_gdt() {
    memset(&gdt, 0, GDT_ENTRIES * sizeof(struct gdt_entry));
    gdt[CS_OFFSET].type = 0b10011011;
    gdt[CS_OFFSET].flags = 0b0010;
    gdt[DATA_OFFSET].type = 0b10010000;
    gdt_descriptor.limit = GDT_ENTRIES * sizeof(struct gdt_entry) - 1;
    gdt_descriptor.gdt = gdt;
    load_gdt(gdt_descriptor);
}