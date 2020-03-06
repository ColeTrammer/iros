#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/statvfs.h>

#include <kernel/fs/file.h>
#include <kernel/fs/file_system.h>
#include <kernel/fs/initrd.h>
#include <kernel/fs/inode.h>
#include <kernel/fs/inode_store.h>
#include <kernel/fs/super_block.h>
#include <kernel/fs/vfs.h>
#include <kernel/hal/output.h>
#include <kernel/hal/timer.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/mem/vm_region.h>
#include <kernel/time/clock.h>
#include <kernel/util/spinlock.h>

static struct file_system fs;
static struct super_block super_block;

/* Should be allocated in super block */
static uint64_t num_files;
static uintptr_t initrd_start;
static struct initrd_file_entry *file_list;

static ino_t inode_count = 1;

static struct file_system fs = { "initrd", 0, &initrd_mount, NULL, NULL };

static struct inode_operations initrd_i_op = { NULL, &initrd_lookup, &initrd_open,     NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                               NULL, NULL,           &initrd_read_all, NULL, NULL };

static struct inode_operations initrd_dir_i_op = { NULL, &initrd_lookup, &initrd_open, NULL, NULL, NULL, NULL, NULL,
                                                   NULL, NULL,           NULL,         NULL, NULL, NULL, NULL, NULL };

static struct file_operations initrd_f_op = { NULL, &initrd_read, NULL, NULL };

static struct file_operations initrd_dir_f_op = { NULL, NULL, NULL, NULL };

struct tnode *initrd_lookup(struct inode *inode, const char *name) {
    if (!inode || !name) {
        return NULL;
    }

    struct tnode_list *list = inode->tnode_list;
    while (list != NULL) {
        assert(list->tnode != NULL);
        assert(list->tnode->name != NULL);
        if (strcmp(list->tnode->name, name) == 0) {
            return list->tnode;
        }
        list = list->next;
    }

    return NULL;
}

struct file *initrd_open(struct inode *inode, int flags, int *error) {
    (void) flags;

    struct initrd_file_entry *entry = (struct initrd_file_entry *) inode->private_data;

    if (!inode) {
        *error = -EINVAL;
        return NULL;
    }

    /* Means we are on root */
    if (!entry) {
        struct file *file = calloc(1, sizeof(struct file));
        file->inode_idenifier = inode->index;
        file->length = 0;
        file->start = 0;
        file->position = 0;
        file->f_op = &initrd_dir_f_op;
        file->device = inode->device;
        file->flags = inode->flags;
        return file;
    }

    struct file *file = calloc(1, sizeof(struct file));
    file->inode_idenifier = inode->index;
    file->length = entry->length;
    file->start = entry->offset;
    file->position = 0;
    file->f_op = &initrd_f_op;
    file->device = inode->device;
    file->flags = inode->flags;
    return file;
}

ssize_t initrd_read(struct file *file, off_t offset, void *buffer, size_t _len) {
    if (file->flags & FS_DIR) {
        return -EISDIR;
    }

    size_t len = MIN(_len, file->length - offset);
    if (len == 0 || *((char *) (initrd_start + file->start + offset)) == '\0') {
        return 0;
    }

    memcpy(buffer, (void *) (initrd_start + file->start + offset), len);
    return (ssize_t) len;
}

int initrd_read_all(struct inode *inode, void *buffer) {
    struct initrd_file_entry *entry = (struct initrd_file_entry *) inode->private_data;

    memcpy(buffer, (void *) (initrd_start + entry->offset), inode->size);
    return 0;
}

struct tnode *initrd_mount(struct file_system *current_fs, char *device_path) {
    struct vm_region *initrd = find_vm_region(VM_INITRD);
    assert(initrd != NULL);
    assert(strlen(device_path) == 0);

    initrd_start = initrd->start;
    num_files = *((int64_t *) initrd_start);
    file_list = (struct initrd_file_entry *) (initrd_start + sizeof(int64_t));

    struct inode *root = calloc(1, sizeof(struct inode));
    root->index = inode_count++;
    root->size = initrd->end - initrd->start;
    root->super_block = &super_block;
    root->flags = FS_DIR;
    root->device = 1;
    root->i_op = &initrd_dir_i_op;
    root->mode = S_IFDIR | 0777;
    root->ref_count = 1;
    root->readable = true;
    root->writeable = false;
    root->access_time = root->modify_time = root->change_time = time_read_clock(CLOCK_REALTIME);
    init_spinlock(&root->lock);

    struct tnode *t_root = create_root_tnode(root);

    current_fs->super_block = &super_block;
    super_block.root = t_root;
    super_block.device = root->device;
    super_block.block_size = PAGE_SIZE;
    super_block.flags = ST_RDONLY;

    struct initrd_file_entry *entry = file_list;
    for (size_t i = 0; i < num_files; i++) {
        struct inode *inode = calloc(1, sizeof(struct inode));
        inode->index = inode_count++;
        inode->size = entry[i].length;
        inode->super_block = &super_block;
        inode->flags = FS_FILE;
        inode->device = root->device;
        inode->mode = S_IFREG | 0777;
        inode->i_op = &initrd_i_op;
        inode->ref_count = 1;
        inode->private_data = entry + i;
        inode->parent = t_root;
        inode->readable = true;
        inode->writeable = true;
        inode->access_time = inode->modify_time = inode->change_time = time_read_clock(CLOCK_REALTIME);
        init_spinlock(&inode->lock);

        struct tnode *to_add = create_tnode(entry[i].name, inode);
        root->tnode_list = add_tnode(root->tnode_list, to_add);
    }

    return t_root;
}

void init_initrd() {
    load_fs(&fs);
}