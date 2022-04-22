#include <assert.h>
#include <string.h>
#include <sys/param.h>

#include <kernel/boot/boot_info.h>
#include <kernel/boot/multiboot2.h>
#include <kernel/hal/hal.h>
#include <kernel/hal/output.h>
#include <kernel/mem/page.h>

static struct boot_info s_boot_info;

struct boot_info *boot_get_boot_info() {
    return &s_boot_info;
}

void init_boot_info_from_multiboot2(const struct multiboot2_info *info) {
    assert((uintptr_t) info < 0x400000ULL);

    multiboot2_for_each_tag(info, tag) {
        if (tag->type == MULTIBOOT2_TAG_BOOT_COMMAND_LINE) {
            MULTIBOOT2_DECLARE_AND_CAST_TAG(command_line_tag, tag, boot_command_line);
            s_boot_info.command_line = command_line_tag->value;
            debug_log("kernel command line: [ %s ]\n", s_boot_info.command_line);
            if (strstr(s_boot_info.command_line, "graphics=0") != 0) {
                kernel_disable_graphics();
            }
            if (strstr(s_boot_info.command_line, "ide_use_default_ports=1") != 0) {
                s_boot_info.ide_use_default_ports = true;
            }
        }

        if (tag->type == MULTIBOOT2_TAG_MEMORY_MAP) {
            MULTIBOOT2_DECLARE_AND_CAST_TAG(memory_map_tag, tag, memory_map);
            s_boot_info.memory_map = memory_map_tag;
        }

        if (tag->type == MULTIBOOT2_TAG_MODULE) {
            MULTIBOOT2_DECLARE_AND_CAST_TAG(module_tag, tag, module);
            s_boot_info.initrd_phys_start = ALIGN_DOWN(module_tag->module_start, PAGE_SIZE);
            s_boot_info.initrd_phys_end = ALIGN_UP(module_tag->module_end, PAGE_SIZE);
            debug_log("kernel module: [ %s, %#.8X, %#.8X ]\n", module_tag->name, module_tag->module_start, module_tag->module_end);
        }
    }
}
