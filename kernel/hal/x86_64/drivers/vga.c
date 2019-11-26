#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <kernel/fs/dev.h>
#include <kernel/hal/output.h>
#include <kernel/mem/page.h>
#include <kernel/mem/vm_region.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/proc/task.h>

#include <kernel/arch/x86_64/asm_utils.h>
#include <kernel/hal/x86_64/drivers/vga.h>

static uint16_t *vga_buffer = (uint16_t*) VGA_PHYS_ADDR;
static enum vga_color fg = VGA_COLOR_LIGHT_GREY;
static enum vga_color bg = VGA_COLOR_BLACK;

static int vga_ioctl(struct device *device, unsigned long request, void *argp) {
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

static intptr_t vga_mmap(struct device *device, void *addr, size_t len, int prot, int flags, off_t offset) {
    if (offset != 0 || len != sizeof(uint16_t) * VGA_HEIGHT * VGA_WIDTH || !(flags & MAP_SHARED)) {
        return -ENODEV;
    }

    (void) device;

    struct task *task = get_current_task();
    struct vm_region *region = calloc(1, sizeof(struct vm_region));
    region->backing_inode = device->inode;
    region->start = addr ? (uintptr_t) addr : 0x100000000LL;
    region->end = region->start + PAGE_SIZE;
    region->type = VM_DEVICE_MEMORY_MAP_DONT_FREE_PHYS_PAGES;
    region->flags = (prot & PROT_WRITE ? VM_WRITE : 0) | VM_USER;
    task->process->process_memory = add_vm_region(task->process->process_memory, region);
    map_phys_page(get_phys_addr((uintptr_t) vga_buffer), region->start, region->flags);
    return (intptr_t) region->start;
}

static struct device_ops vga_ops = {
    NULL, NULL, NULL, NULL, NULL, NULL, vga_ioctl, NULL, vga_mmap
};

static struct device vga_device = {
    0x1234, S_IFCHR, "fb0", false, &vga_ops, NULL, NULL
};

void vga_enable_cursor() {
	VGA_RUN_COMMAND(VGA_ENABLE_CURSOR_START, (inb(VGA_DATA) & 0xC0) | VGA_CURSOR_Y_START);
	VGA_RUN_COMMAND(VGA_ENABLE_CURSOR_END, (inb(VGA_DATA) & 0xE0) | VGA_CURSOR_Y_END);
}

void vga_disable_cursor() {
	VGA_RUN_COMMAND(VGA_ENABLE_CURSOR_START, VGA_CURSOR_DISABLE);
}

void update_vga_buffer() {
    vga_buffer = create_phys_addr_mapping(VGA_PHYS_ADDR);
    debug_log("VGA Buffer Updated: [ %#.16lX ]\n", (uintptr_t) vga_buffer);

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

    VGA_RUN_COMMAND(VGA_SET_CURSOR_LOW, (uint8_t) (pos & 0xFF));
    VGA_RUN_COMMAND(VGA_SET_CURSOR_HIGH, (uint8_t) ((pos >> 8) & 0xFF));
}

void init_vga_device() {
    dev_add(&vga_device, vga_device.name);
}