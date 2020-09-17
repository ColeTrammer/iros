#include <kernel/fs/dev.h>
#include <kernel/hal/block.h>
#include <kernel/hal/gpt.h>
#include <kernel/hal/mbr.h>
#include <kernel/hal/output.h>
#include <kernel/hal/partition.h>
#include <kernel/mem/phys_page.h>
#include <kernel/mem/vm_allocator.h>

void mbr_partition_device(struct block_device *block_device) {
    mutex_lock(&block_device->device->lock);
    struct phys_page *first_page = block_device->op->read_page(block_device, 0);
    mutex_unlock(&block_device->device->lock);

    struct mbr_table *mbr = create_phys_addr_mapping(first_page->phys_addr);
    if (mbr->boot_signature != 0xAA55) {
        debug_log("Drive has no MBR: [ %lu ]\n", block_device->device->device_number);
        goto done;
    }

    debug_log("Drive has MBR: [ %lu ]\n", block_device->device->device_number);
    for (int i = 0; i < MBR_MAX_PARTITIONS; i++) {
        struct mbr_partition *partition = &mbr->partitions[i];
        if (i == 0 && partition->partition_type == GPT_MBR_TYPE) {
            gpt_partition_device(block_device);
            goto done;
        }

        if (partition->partition_type != 0) {
            debug_log("MBR partition: [ %d, %u, %u, %u, %u ]\n", i + 1, partition->drive_attributes, partition->partition_type,
                      partition->lba_start, partition->sector_count);
            create_and_register_partition_device(block_device, partition->sector_count, partition->lba_start, i + 1);
        }
    }

done:
    drop_phys_page(first_page);
}
