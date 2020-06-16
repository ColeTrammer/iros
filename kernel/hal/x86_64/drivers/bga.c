#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <kernel/fs/dev.h>
#include <kernel/hal/output.h>
#include <kernel/hal/x86_64/drivers/bga.h>
#include <kernel/mem/page_frame_allocator.h>
#include <kernel/mem/phys_vm_object.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/mem/vm_region.h>
#include <kernel/proc/task.h>

struct bga_data {
    uintptr_t frame_buffer;
    uint16_t x_res;
    uint16_t y_res;
};

static int bga_ioctl(struct device *device, unsigned long request, void *argp);
static intptr_t bga_mmap(struct device *device, void *addr, size_t len, int prot, int flags, off_t offset);

static struct bga_data data = { 0 };

struct device_ops bga_ops = { .ioctl = bga_ioctl, .mmap = bga_mmap };

struct device bga_device = { .device_number = 0x00600, .type = S_IFCHR, .ops = &bga_ops, .private = &data, .lock = SPINLOCK_INITIALIZER };

static int bga_ioctl(struct device *device, unsigned long request, void *argp) {
    assert(device);

    switch (request) {
        case SSRES: {
            struct screen_res *res = argp;
            bga_write(BGA_INDEX_ENABLE, BGA_VBE_DISABLED);
            bga_write(BGA_INDEX_X_RES, res->x);
            bga_write(BGA_INDEX_Y_RES, res->y);
            bga_write(BGA_INDEX_VIRT_WIDTH, res->x);
            bga_write(BGA_INDEX_VIRT_HEIGHT, res->y * 2);
            bga_write(BGA_INDEX_X_OFFSET, 0);
            bga_write(BGA_INDEX_Y_OFFSET, 0);
            bga_write(BGA_INDEX_BPP, BGA_BPP_32);
            bga_write(BGA_INDEX_BANK, 0);
            bga_write(BGA_INDEX_ENABLE, BGA_VBE_ENABLED | BGA_LFB_ENABLED);
            data.x_res = res->x;
            data.y_res = res->y;
            return 0;
        }
        case SSWAPBUF: {
            uintptr_t new_buffer = (uintptr_t) argp;
            bga_write(BGA_INDEX_Y_OFFSET, (new_buffer - find_user_vm_region_by_addr(new_buffer)->start) / data.x_res / sizeof(uint32_t));
            return 0;
        }
        default:
            return -ENOTTY;
    }
}

static intptr_t bga_mmap(struct device *device, void *addr, size_t len, int prot, int flags, off_t offset) {
    if (offset != 0 || len != sizeof(uint32_t) * data.x_res * data.y_res * 2 || !(flags & MAP_SHARED)) {
        return -ENODEV;
    }

    size_t total_size = sizeof(uint32_t) * (size_t) data.x_res * (size_t) data.y_res * (size_t) 2;
    debug_log("Framebuffer total size: [ %lu ]\n", total_size);

    if (!device->inode->vm_object) {
        device->inode->vm_object = vm_create_phys_object(data.frame_buffer, total_size, inode_on_kill, device->inode);
    } else {
        bump_vm_object(device->inode->vm_object);
    }

    struct vm_region *region = map_region(addr, len, prot, VM_DEVICE_MEMORY_MAP_DONT_FREE_PHYS_PAGES);
    region->vm_object = device->inode->vm_object;
    region->vm_object_offset = 0;
    region->flags |= VM_SHARED;

    int ret = vm_map_region_with_object(region);
    if (ret < 0) {
        return (intptr_t) ret;
    }

    // FIXME: this should probably be done during the device's construction
    for (uintptr_t i = region->start; i < region->end; i += PAGE_SIZE) {
        mark_used(data.frame_buffer + (i - region->start), PAGE_SIZE);
    }

    return (intptr_t) region->start;
}

void init_bga(struct pci_configuration *config) {
    debug_log("Detected bga device: [ %#.8X ]\n", config->bar[0] & ~0xF);

    data.frame_buffer = config->bar[0] & ~0xF;
    dev_register(&bga_device);
}
