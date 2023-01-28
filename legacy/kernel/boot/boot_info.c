#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include <kernel/boot/boot_info.h>
#include <kernel/boot/multiboot2.h>
#include <kernel/boot/xen.h>
#include <kernel/hal/hal.h>
#include <kernel/hal/output.h>
#include <kernel/mem/page.h>

// #define BOOT_INFO_DEBUG

static char s_command_line_buffer[2048];
static struct boot_info s_boot_info;

struct boot_info *boot_get_boot_info() {
    return &s_boot_info;
}

static void init_command_line(const char *command_line) {
    strncpy(s_command_line_buffer, command_line, sizeof(s_command_line_buffer) - 1);
    s_boot_info.command_line = s_command_line_buffer;
#ifdef BOOT_INFO_DEBUG
    debug_log("kernel command line: [ %s ]\n", s_boot_info.command_line);
#endif /* BOOT_INFO_DEBUG */
    if (strstr(s_boot_info.command_line, "graphics=0") != 0) {
        kernel_disable_graphics();
    }
    if (strstr(s_boot_info.command_line, "ide_use_default_ports=1") != 0) {
        s_boot_info.ide_use_default_ports = true;
    }
    s_boot_info.serial_debug = true;
    if (strstr(s_boot_info.command_line, "disable_serial_debug=1")) {
        s_boot_info.serial_debug = false;
    }
    if (strstr(s_boot_info.command_line, "redirect_start_stdio_to_serial=1")) {
        s_boot_info.redirect_start_stdio_to_serial = true;
    }
    if (strstr(s_boot_info.command_line, "poweroff_on_panic=1")) {
        s_boot_info.poweroff_on_panic = true;
    }
}

void init_boot_info_from_multiboot2(const struct multiboot2_info *info) {
    assert((uintptr_t) info < 0x400000ULL);

    multiboot2_for_each_tag(info, tag) {
        if (tag->type == MULTIBOOT2_TAG_BOOT_COMMAND_LINE) {
            MULTIBOOT2_DECLARE_AND_CAST_TAG(command_line_tag, tag, boot_command_line);
            init_command_line(command_line_tag->value);
        }

        if (tag->type == MULTIBOOT2_TAG_MEMORY_MAP) {
            MULTIBOOT2_DECLARE_AND_CAST_TAG(memory_map_tag, tag, memory_map);
            s_boot_info.memory_map = memory_map_tag;
        }

        if (tag->type == MULTIBOOT2_TAG_MODULE) {
            MULTIBOOT2_DECLARE_AND_CAST_TAG(module_tag, tag, module);
            s_boot_info.initrd_phys_start = ALIGN_DOWN(module_tag->module_start, PAGE_SIZE);
            s_boot_info.initrd_phys_end = ALIGN_UP(module_tag->module_end, PAGE_SIZE);
#ifdef BOOT_INFO_DEBUG
            debug_log("kernel module: [ %s, %#.8X, %#.8X ]\n", module_tag->name, module_tag->module_start, module_tag->module_end);
#endif /* BOOT_INFO_DEBUG */
        }
    }
}

void init_boot_info_from_xen(const struct xen_boot_info *info) {
    if (info->magic != XEN_BOOT_MAGIC) {
        debug_log("Incorrect Xen boot magic: [ %8.X ]\n", info->magic);
        abort();
    }

#ifdef BOOT_INFO_DEBUG
    debug_log("Found Xen boot info: [ %p ]\n", info);
#endif /* BOOT_INFO_DEBUG */

    bool found_initrd = false;

    uint32_t module_count = info->module_count;
    const struct xen_module_entry *modules = (void *) (uintptr_t) info->modules;
    for (uint32_t i = 0; i < module_count; i++) {
        const struct xen_module_entry *module = &modules[i];
#ifdef BOOT_INFO_DEBUG
        debug_log("Xen Module: [ %.16" PRIX64 ", %" PRIu64 " ]\n", module->addr, module->size);
#endif /* BOOT_INFO_DEBUG */

        found_initrd = true;
        s_boot_info.initrd_phys_start = ALIGN_DOWN(module->addr, PAGE_SIZE);
        s_boot_info.initrd_phys_end = ALIGN_UP(module->addr + module->size, PAGE_SIZE);
        break;
    }

    if (!found_initrd) {
        debug_log("Failed to locate initrd\n");
        abort();
    }

    init_command_line((void *) (uintptr_t) info->command_line);

    s_boot_info.memory_map = (void *) (uintptr_t) info->memory_map;
    s_boot_info.memory_map_count = info->memory_map_entry_count;
}

void init_boot_info(int boot_info_type, const void *info) {
    s_boot_info.boot_info_type = boot_info_type;
    switch (boot_info_type) {
        case BOOT_INFO_MULTIBOOT2:
            init_boot_info_from_multiboot2(info);
            break;
        case BOOT_INFO_XEN:
            init_boot_info_from_xen(info);
            break;
        default:
            debug_log("Failed to read boot info: unknown type: [ %d ]\n", boot_info_type);
            abort();
    }
}
