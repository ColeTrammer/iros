#ifndef _KERNEL_FS_EXT2_H
#define _KERNEL_FS_EXT2_H 1

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include <kernel/fs/file_system.h>
#include <kernel/fs/tnode.h>
#include <kernel/util/bitset.h>
#include <kernel/util/uuid.h>

#define EXT2_SUPER_BLOCK_OFFSET 1024
#define EXT2_SUPER_BLOCK_SIZE   1024

#define EXT2_ROOT_INODE 2

#define EXT2_MAX_FILE_NAME_LENGTH 255

struct ext2_raw_super_block {
    uint32_t num_inodes;
    uint32_t num_blocks;
    uint32_t num_reserved_blocks;
    uint32_t num_unallocated_blocks;
    uint32_t num_unallocated_inodes;
    uint32_t super_block_block_number;
    uint32_t shifted_blck_size;
    uint32_t shifted_fragment_size;
    uint32_t num_blocks_in_block_group;
    uint32_t num_fragments_in_block_group;
    uint32_t num_inodes_in_block_group;
    uint32_t last_mount_time;
    uint32_t last_written_time;
    uint16_t num_fsck;
    uint16_t num_til_fsck;
    uint16_t ext2_sig;
#define EXT2_FS_STATE_CLEAN 1
#define EXT2_FS_STATE_ERROR 2
    uint16_t fs_state;
#define EXT2_FS_IGNORE_ERROR         1
#define EXT2_FS_REMOUNT_AS_READ_ONLY 2
#define EXT2_FS_KERNEL_PANIC         3
    uint16_t what_to_do_if_error;
    uint16_t version_minor;
    uint32_t fsck_time;
    uint32_t fsck_interval;
#define EXT2_CREATOR_LINUX     0
#define EXT2_CREATOR_GNU_HURD  1
#define EXT2_CREATOR_MASIX     2
#define EXT2_CREATOR_FREEBSD   3
#define EXT2_CREATOR_BSD_LITES 4
    uint32_t os_id;
    uint32_t version_major;
    uint16_t user_id_for_reserved_blocks;
    uint16_t group_id_for_reserved_blocks;
    uint32_t first_non_reserved_inode;
    uint16_t inode_size;
    uint16_t block_group_of_this_super_block;
#define EXT2_PREALLOCATE_BLOCKS_FOR_DIRECTORIES 1
#define EXT2_AFS_SERVER_INODES                  2
#define EXT2_FS_JOURNAL                         4
#define EXT2_EXTENDED_INODES                    8
#define EXT2_FS_CAN_RESIZE                      16
#define EXT2_DIRECTORIES_USE_HASH_INDEX         32
    uint32_t optional_features_present;
#define EXT2_COMPRESSION           1
#define EXT2_DIRECTORIES_HAVE_TYPE 2
#define EXT2_NEEDS_JOURNAL_REPLAY  4
#define EXT2_USES_JOURNAL_DEVICE   8
    uint32_t required_features_present;
#define EXT2_SPARSE_SUPERBLOCKS_AND_GROUP_DESCRIPTORS 1
#define EXT2_64_BIT_FILE_SIZE                         2
#define EXT2_BINARY_TREE_DIRECTORIES                  4
    uint32_t required_features_for_write;
    struct uuid fs_uuid;
    uint8_t volumne_name[16];
    uint8_t path_of_last_mount[64];
    uint32_t compression_algorithm;
    uint8_t num_blocks_to_preallocate_for_files;
    uint8_t num_blocks_to_preallocate_for_directories;
    uint8_t unused0[2];
    uint8_t journal_id[16];
    uint32_t journal_inode;
    uint32_t journal_device;
    uint32_t head_of_orphan_inode_list;
    uint8_t unused1[786];
} __attribute__((packed));

struct raw_block_group_descriptor {
    uint32_t block_usage_bitmap_block_address;
    uint32_t inode_usage_bitmap_block_address;
    uint32_t inode_table_block_address;
    uint16_t num_unallocated_blocks;
    uint16_t num_unallocated_inodes;
    uint16_t num_directories;
    uint16_t padding;
    uint8_t unused[12];
} __attribute__((packed));

struct raw_inode {
    uint16_t mode;
    uint16_t uid;
    uint32_t size;
    uint32_t atime;
    uint32_t ctime;
    uint32_t mtime;
    uint32_t dtime;
    uint16_t gid;
    uint16_t link_count;
    uint32_t sectors;
    uint32_t flags;
    uint32_t os_specific_1;
#define EXT2_SINGLY_INDIRECT_BLOCK_INDEX 12
#define EXT2_DOUBLY_INDIRECT_BLOCK_INDEX 13
#define EXT2_TRIPLY_INDRIECT_BLOCK_INDEX 14
    uint32_t block[15];
    uint32_t generation;
    uint32_t file_acl;
    uint32_t dir_acl;
    uint32_t faddr;
    uint8_t os_specific_2[12];
} __attribute__((packed)) __attribute__((aligned(sizeof(uint32_t))));

struct raw_dirent {
    uint32_t ino;
    uint16_t size;
    uint8_t name_length;

#define EXT2_DIRENT_TYPE_UNKNOWN          0
#define EXT2_DIRENT_TYPE_REGULAR          1
#define EXT2_DIRENT_TYPE_DIRECTORY        2
#define EXT2_DIRENT_TYPE_CHARACTER_DEVICE 3
#define EXT2_DIRENT_TYPE_BLOCK            4
#define EXT2_DIRENT_TYPE_FIFO             5
#define EXT2_DIRENT_TYPE_SOCKET           6
#define EXT2_DIRENT_TYPE_SYMBOLIC_LINK    7
    uint8_t type;
    char name[];
} __attribute__((packed));

#define EXT2_NEXT_DIRENT(dirent) ((struct raw_dirent *) (((uintptr_t)(dirent)) + (dirent)->size))

struct ext2_block_group {
    size_t index;
    struct raw_block_group_descriptor *blk_desc;
    struct bitset block_bitset;
    struct bitset inode_bitset;
    bool initialized;
};

struct ext2_sb_data {
    struct ext2_raw_super_block *sb;
    struct raw_block_group_descriptor *blk_desc_table;
    struct ext2_block_group *block_groups;
    size_t num_block_groups;
};

struct ext2_data_block_info {
    uint32_t block_index;
    uint32_t indirect_indices[3];
    uint32_t indirect_block_indices[3];
    uint32_t *indirect_block_data[3];
    int level;
    bool indirect : 1;
};

struct ext2_block_iterator {
    struct super_block *super_block;
    struct inode *inode;
    struct raw_inode *raw_inode;
    uint32_t *indirect[3];
    uint32_t indexes[3];
    uint32_t limits[3];
    uint32_t block_indices[3];
    size_t byte_offset;
    int level;
    bool at_indirect : 1;
    bool write_mode : 1;
};

void ext2_init_block_iterator(struct ext2_block_iterator *iter, struct inode *inode, bool write_mode);
int ext2_block_iterator_set_block_offset(struct ext2_block_iterator *iter, size_t offset);
int ext2_block_iterator_next_info(struct ext2_block_iterator *iter, struct ext2_data_block_info *info);
int ext2_block_iterator_next(struct ext2_block_iterator *iter, uint32_t *block);
void ext2_kill_block_iterator(struct ext2_block_iterator *iter);

struct inode *ext2_lookup(struct inode *inode, const char *name);
struct file *ext2_open(struct inode *inode, int flags, int *error);
int ext2_close(struct file *file);
ssize_t ext2_read(struct file *file, off_t offset, void *buffer, size_t len);
ssize_t ext2_iread(struct inode *inode, void *buffer, size_t len, off_t offset);
ssize_t ext2_write(struct file *file, off_t offset, const void *buffer, size_t len);
int ext2_stat(struct inode *inode, struct stat *stat_struct);
struct inode *ext2_mkdir(struct tnode *tparent, const char *name, mode_t mode, int *error);
struct inode *ext2_mknod(struct tnode *tparent, const char *name, mode_t mode, dev_t device, int *error);
int ext2_unlink(struct tnode *tnode);
int ext2_rmdir(struct tnode *tnode);
int ext2_mount(struct block_device *device, unsigned long flags, const void *data, struct super_block **super_block);
int ext2_umount(struct super_block *super_block);
int ext2_determine_fsid(struct block_device *block_device, struct block_device_id *result);
int ext2_chmod(struct inode *inode, mode_t mode);
int ext2_chown(struct inode *inode, uid_t uid, gid_t gid);
int ext2_utimes(struct inode *inode, const struct timespec *times);
int ext2_rename(struct tnode *tnode, struct tnode *new_parent, const char *new_name);
struct inode *ext2_symlink(struct tnode *tnode, const char *name, const char *target, int *error);
int ext2_link(struct tnode *tnode, const char *name, const struct tnode *target);
int ext2_truncate(struct inode *inode, off_t length);
int ext2_read_all(struct inode *inode, void *buffer);
void ext2_on_inode_destruction(struct inode *inode);
int ext2_sync_inode(struct inode *inode);
int ext2_sync_super_block(struct super_block *sb);

#endif /* _KERNEL_FS_EXT2_H */
