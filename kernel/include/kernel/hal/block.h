#ifndef _KERNEL_HAL_BLOCK_H
#define _KERNEL_HAL_BLOCK_H 1

#include <sys/types.h>

#include <kernel/util/list.h>
#include <kernel/util/uuid.h>

struct block_device;
struct fs_device;
struct hash_map;
struct phys_page;

enum block_device_type {
    BLOCK_TYPE_DISK,
    BLOCK_TYPE_PARTITION,
};

enum block_device_id_type {
    BLOCK_ID_TYPE_NONE,
    BLOCK_ID_TYPE_UUID,
    BLOCK_ID_TYPE_MBR,
};

struct block_device_id {
    enum block_device_id_type type;
    union {
        struct uuid uuid;
        uint32_t mbr_id;
    };
};

static inline struct block_device_id block_device_id_none(void) {
    return (struct block_device_id) { .type = BLOCK_ID_TYPE_NONE };
}

static inline struct block_device_id block_device_id_mbr(uint32_t id) {
    return (struct block_device_id) { .type = BLOCK_ID_TYPE_MBR, .mbr_id = id };
}

static inline struct block_device_id block_device_id_uuid(struct uuid uuid) {
    return (struct block_device_id) { .type = BLOCK_ID_TYPE_UUID, .uuid = uuid };
}

struct block_device_info {
    enum block_device_type type;
    struct block_device_id disk_id;
    struct block_device_id partition_id;
    struct block_device_id filesystem_type_id;
    struct block_device_id filesystem_id;
};

static inline struct block_device_info block_device_info_none(enum block_device_type type) {
    return (struct block_device_info) { .type = type };
}

struct block_device_ops {
    blkcnt_t (*read)(struct block_device *self, void *buf, blkcnt_t block_count, off_t block_offset);
    blkcnt_t (*write)(struct block_device *self, const void *buf, blkcnt_t block_count, off_t block_offset);
    struct phys_page *(*read_page)(struct block_device *self, off_t block_offset);
    int (*sync_page)(struct block_device *self, struct phys_page *page);
};

struct block_device {
    struct fs_device *device;
    blkcnt_t block_count;
    blksize_t block_size;
    off_t partition_offset;
    int partition_number;
    struct hash_map *block_hash_map;
    struct list_node lru_list;
    struct block_device_ops *op;
    void *private_data;
    struct list_node list;
    struct block_device_info info;
};

struct phys_page *block_generic_read_page(struct block_device *self, off_t block_offset);
int block_generic_sync_page(struct block_device *self, struct phys_page *page);

void block_trim_cache(void);
struct phys_page *block_allocate_phys_page(struct block_device *block_device);
struct block_device *create_block_device(blkcnt_t block_count, blksize_t block_size, struct block_device_info info,
                                         struct block_device_ops *op, void *private_data);
void block_register_device(struct block_device *block_device, dev_t device_number);
struct list_node *block_device_list(void);
int block_show_device_id(struct block_device_id id, char *buffer, size_t buffer_length);
int block_show_device(struct block_device *block_device, char *buffer, size_t buffer_length);

const char *block_device_type_to_string(enum block_device_type type);

static inline bool block_is_root_device(struct block_device *block_device) {
    return block_device->partition_number == 0;
}

#define block_for_each_device(name) list_for_each_entry(block_device_list(), name, struct block_device, list)

#endif /* _KERNEL_HAL_BLOCK_H */
