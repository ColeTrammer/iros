#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/statvfs.h>

#include <kernel/fs/cached_dirent.h>
#include <kernel/fs/file.h>
#include <kernel/fs/file_system.h>
#include <kernel/fs/initrd.h>
#include <kernel/fs/inode.h>
#include <kernel/fs/super_block.h>
#include <kernel/fs/vfs.h>
#include <kernel/hal/output.h>
#include <kernel/hal/timer.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/mem/vm_region.h>
#include <kernel/time/clock.h>
#include <kernel/util/init.h>
#include <kernel/util/spinlock.h>

static struct file_system fs;
static struct super_block super_block;

/* Should be allocated in super block */
static uint64_t num_files;
static uintptr_t initrd_start;
static struct initrd_file_entry *file_list;

static ino_t inode_count = 1;

static struct file_system fs = {
    .name = "initrd",
    .mount = &initrd_mount,
};

static struct inode_operations initrd_i_op = {
    .lookup = &initrd_lookup,
    .open = &initrd_open,
    .mmap = &fs_default_mmap,
    .read_all = &initrd_read_all,
    .read = &initrd_iread,
};

static struct inode_operations initrd_dir_i_op = {
    .lookup = &initrd_lookup,
    .open = &initrd_open,
};

static struct file_operations initrd_f_op = {
    .read = &initrd_read,
};

static struct file_operations initrd_dir_f_op = {};

struct inode *initrd_lookup(struct inode *inode, const char *name) {
    if (!inode || !name) {
        return NULL;
    }

    return fs_lookup_in_cache(inode->dirent_cache, name);
}

struct file *initrd_open(struct inode *inode, int flags, int *error) {
    if (!inode) {
        *error = -EINVAL;
        return NULL;
    }

    /* Means we are on root */
    if (inode->flags & FS_DIR) {
        return fs_create_file(inode, FS_DIR, 0, flags, &initrd_dir_f_op, NULL);
    }

    return fs_create_file(inode, FS_FILE, 0, flags, &initrd_f_op, NULL);
}

ssize_t initrd_iread(struct inode *inode, void *buffer, size_t _len, off_t offset) {
    struct initrd_file_entry *entry = inode->private_data;
    return fs_do_read(buffer, offset, _len, (void *) (initrd_start + entry->offset), inode->size);
}

ssize_t initrd_read(struct file *file, off_t offset, void *buffer, size_t _len) {
    if (file->flags & FS_DIR) {
        return -EISDIR;
    }

    struct inode *inode = fs_file_inode(file);
    return initrd_iread(inode, buffer, _len, offset);
}

int initrd_read_all(struct inode *inode, void *buffer) {
    struct initrd_file_entry *entry = (struct initrd_file_entry *) inode->private_data;

    memcpy(buffer, (void *) (initrd_start + entry->offset), inode->size);
    return 0;
}

struct inode *initrd_mount(struct file_system *current_fs, struct device *device) {
    struct vm_region *initrd = find_vm_region(VM_INITRD);
    assert(initrd != NULL);
    assert(!device);

    initrd_start = initrd->start;
    num_files = *((int64_t *) initrd_start);
    file_list = (struct initrd_file_entry *) (initrd_start + sizeof(int64_t));

    super_block.fsid = 1;
    super_block.block_size = PAGE_SIZE;
    super_block.flags = ST_RDONLY;

    struct inode *root =
        fs_create_inode(&super_block, inode_count++, 0, 0, S_IFDIR | 0777, initrd->end - initrd->start, &initrd_dir_i_op, NULL);

    current_fs->super_block = &super_block;
    super_block.root = root;

    struct initrd_file_entry *entry = file_list;
    for (size_t i = 0; i < num_files; i++) {
        struct inode *inode = fs_create_inode(&super_block, inode_count++, 0, 0, S_IFREG | 0777, entry[i].length, &initrd_i_op, entry + i);
        fs_put_dirent_cache(root->dirent_cache, inode, entry[i].name, strlen(entry[i].name));
    }

    return root;
}

static void init_initrd() {
    load_fs(&fs);
}
INIT_FUNCTION(init_initrd, fs);
