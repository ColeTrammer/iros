#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <kernel/arch/x86_64/asm_utils.h>
#include <kernel/fs/dev.h>
#include <kernel/hal/hal.h>
#include <kernel/hal/hw_device.h>
#include <kernel/hal/isa_driver.h>
#include <kernel/hal/output.h>
#include <kernel/hal/x86_64/drivers/vga.h>
#include <kernel/mem/page.h>
#include <kernel/mem/phys_vm_object.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/mem/vm_region.h>
#include <kernel/proc/task.h>
#include <kernel/util/init.h>

static uint16_t *vga_buffer = (uint16_t *) VGA_PHYS_ADDR;
static enum vga_color fg = VGA_COLOR_LIGHT_GREY;
static enum vga_color bg = VGA_COLOR_BLACK;

static int vga_ioctl(struct fs_device *device, unsigned long request, void *argp) {
    assert(device);

    switch (request) {
        case SGWIDTH: {
            int *wp = argp;
            *wp = VGA_WIDTH;
            return 0;
        }
        case SGHEIGHT: {
            int *hp = argp;
            *hp = VGA_HEIGHT;
            return 0;
        }
        case SSCURSOR: {
            struct cursor_pos *cp = argp;
            set_vga_cursor(cp->cp_row, cp->cp_col);
            return 0;
        }
        case SECURSOR: {
            vga_enable_cursor();
            return 0;
        }
        case SDCURSOR: {
            vga_disable_cursor();
            return 0;
        }
        default:
            return -ENOTTY;
    }
}

static intptr_t vga_mmap(struct fs_device *device, void *addr, size_t len, int prot, int flags, off_t offset) {
    if (offset != 0 || len != sizeof(uint16_t) * VGA_HEIGHT * VGA_WIDTH || !(flags & MAP_SHARED)) {
        return -ENODEV;
    }

    if (!device->vm_object) {
        device->vm_object = vm_create_phys_object(get_phys_addr((uintptr_t) vga_buffer), PAGE_SIZE, device_on_kill, device);
    } else {
        bump_vm_object(device->vm_object);
    }

    struct vm_region *region = map_region(addr, len, prot, flags, VM_DEVICE_MEMORY_MAP_DONT_FREE_PHYS_PAGES);
    region->vm_object = device->vm_object;
    region->vm_object_offset = 0;
    region->flags |= VM_SHARED;

    int ret = vm_map_region_with_object(region);
    if (ret < 0) {
        return (intptr_t) ret;
    }

    return (intptr_t) region->start;
}

static struct fs_device_ops vga_ops = { .ioctl = vga_ioctl, .mmap = vga_mmap };

static struct fs_device vga_device = { .device_number = 0x00600, .type = S_IFCHR, .ops = &vga_ops, .lock = MUTEX_INITIALIZER };

void vga_enable_cursor() {
    VGA_RUN_COMMAND(VGA_ENABLE_CURSOR_START, (inb(VGA_DATA) & 0xC0) | VGA_CURSOR_Y_START);
    VGA_RUN_COMMAND(VGA_ENABLE_CURSOR_END, (inb(VGA_DATA) & 0xE0) | VGA_CURSOR_Y_END);
}

void vga_disable_cursor() {
    VGA_RUN_COMMAND(VGA_ENABLE_CURSOR_START, VGA_CURSOR_DISABLE);
}

void update_vga_buffer() {
    vga_buffer = create_phys_addr_mapping(VGA_PHYS_ADDR);
    vga_enable_cursor();
}

void set_vga_foreground(enum vga_color _fg) {
    fg = _fg;
}

void set_vga_background(enum vga_color _bg) {
    bg = _bg;
}

void swap_vga_colors() {
    enum vga_color temp = fg;
    fg = bg;
    bg = temp;
}

void write_vga_buffer(size_t row, size_t col, uint16_t c, bool raw_copy) {
    vga_buffer[VGA_INDEX(row, col)] = raw_copy ? c : VGA_ENTRY(c & 0xFF, fg, bg);
}

uint16_t get_vga_buffer(size_t row, size_t col) {
    return vga_buffer[VGA_INDEX(row, col)];
}

void set_vga_cursor(size_t row, size_t col) {
    uint16_t pos = (uint16_t) VGA_INDEX(row, col);

    if (pos >= VGA_WIDTH * VGA_HEIGHT) {
        vga_disable_cursor();
        return;
    }

    VGA_RUN_COMMAND(VGA_SET_CURSOR_LOW, (uint8_t)(pos & 0xFF));
    VGA_RUN_COMMAND(VGA_SET_CURSOR_HIGH, (uint8_t)((pos >> 8) & 0xFF));
}

static void detect_vga_device(struct hw_device *parent) {
    struct hw_device *device = create_hw_device("VGA Graphics", parent, hw_device_id_isa(), &vga_device);
    device->status = HW_STATUS_ACTIVE;
}

static struct isa_driver vga_driver = {
    .name = "x86 VGA Driver",
    .detect_devices = detect_vga_device,
};

static void init_vga_device(void) {
    if (!kernel_use_graphics()) {
        register_isa_driver(&vga_driver);
    }
}
INIT_FUNCTION(init_vga_device, driver);
