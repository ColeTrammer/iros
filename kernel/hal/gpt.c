#include <assert.h>
#include <kernel/fs/dev.h>
#include <kernel/hal/block.h>
#include <kernel/hal/gpt.h>
#include <kernel/hal/output.h>
#include <kernel/hal/partition.h>
#include <kernel/mem/page.h>
#include <kernel/mem/phys_page.h>
#include <kernel/mem/vm_allocator.h>

void gpt_partition_device(struct block_device *block_device) {
    mutex_lock(&block_device->device->lock);

    off_t offset = block_device->block_size;
    struct phys_page *page = block_device->op->read_page(block_device, (offset / PAGE_SIZE) * (PAGE_SIZE / block_device->block_size));

    struct gpt_header *gpt = create_phys_addr_mapping(page->phys_addr) + (offset % PAGE_SIZE);
    if (gpt->signature != GPT_SIGNATURE) {
        debug_log("Drive has no GPT: [ %lu ]\n", block_device->device->device_number);
        goto done;
    }

    uint32_t partition_entry_count = gpt->partition_entry_count;
    uint32_t partition_entry_size = gpt->partition_entry_size;
    char uuid_string[UUID_STRING_MAX];
    {
        uuid_to_string(gpt->disk_uuid, uuid_string, sizeof(uuid_string));
        debug_log("Drive has GPT: [ %s, %" PRIu64 ", %u, %u ]\n", uuid_string, gpt->partition_entry_start, partition_entry_count,
                  gpt->partition_entry_size);
        block_device->info.disk_id = block_device_id_uuid(gpt->disk_uuid);

        if (PAGE_SIZE % gpt->partition_entry_size != 0) {
            debug_log("GPT partition size is wrong, should be 128 bytes\n");
            goto done;
        }

        if (partition_entry_count * gpt->partition_entry_size > block_device->block_size * 32) {
            debug_log("GPT has more partitions than should be possible\n");
            goto done;
        }
    }
    free_phys_addr_mapping(gpt);
    gpt = NULL;

    size_t old_page = offset / PAGE_SIZE;
    offset = 2 * block_device->block_size;
    int partition_number = 1;
    for (size_t i = 0; i < partition_entry_count; i++) {
        size_t new_page = offset / PAGE_SIZE;
        if (old_page != new_page) {
            old_page = new_page;
            drop_phys_page(page);
            page = block_device->op->read_page(block_device, (offset / PAGE_SIZE) * (PAGE_SIZE / block_device->block_size));
            assert(page);
        }

        void *gpt_partition_base = create_phys_addr_mapping(page->phys_addr);
        struct gpt_partition_entry *gpt_partition = gpt_partition_base + (offset % PAGE_SIZE);
        if (!uuid_equals(gpt_partition->type_uuid, UUID_ZEROES)) {
            char uuid_type_string[UUID_STRING_MAX];
            uuid_to_string(gpt_partition->type_uuid, uuid_type_string, sizeof(uuid_type_string));
            uuid_to_string(gpt_partition->partition_uuid, uuid_string, sizeof(uuid_string));
            debug_log("GPT partition: [ %s, %s, %" PRIu64 ", %" PRIu64 " ]\n", uuid_type_string, uuid_string, gpt_partition->start,
                      gpt_partition->end);

            struct block_device_info info = {
                .type = BLOCK_TYPE_PARTITION,
                .disk_id = block_device->info.disk_id,
                .partition_id = block_device_id_uuid(gpt_partition->partition_uuid),
                .filesystem_type_id = block_device_id_uuid(gpt_partition->type_uuid),
            };
            create_and_register_partition_device(block_device, gpt_partition->end - gpt_partition->start + 1, gpt_partition->start,
                                                 partition_number++, info);
        }
        free_phys_addr_mapping(gpt_partition_base);
        offset += partition_entry_size;
    }

done:
    if (gpt) {
        free_phys_addr_mapping(gpt);
    }
    mutex_unlock(&block_device->device->lock);
    drop_phys_page(page);
}
