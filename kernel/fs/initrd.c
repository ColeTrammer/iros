#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/param.h>
#include <errno.h>
#include <dirent.h>

#include <kernel/fs/file.h>
#include <kernel/fs/inode.h>
#include <kernel/fs/inode_store.h>
#include <kernel/fs/initrd.h>
#include <kernel/fs/file_system.h>
#include <kernel/fs/vfs.h>
#include <kernel/fs/super_block.h>
#include <kernel/mem/vm_region.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/hal/output.h>
#include <kernel/util/spinlock.h>

static struct file_system fs;
static struct super_block super_block;

/* Should be allocated in super block */
static uint64_t num_files;
static uintptr_t initrd_start;
static struct initrd_file_entry *file_list;

static struct file_system fs = {
    "initrd", 0, &initrd_mount, NULL, NULL
};

static struct inode_operations initrd_i_op = {
    &initrd_lookup, &initrd_open, &initrd_stat
};

static struct inode_operations initrd_dir_i_op = {
    &initrd_lookup, &initrd_open, &initrd_stat
};

static struct file_operations initrd_f_op = {
    NULL, &initrd_read, NULL
};

static struct file_operations initrd_dir_f_op = {
    NULL, NULL, NULL
};

struct tnode *initrd_lookup(struct inode *inode, const char *name) {
    /* Assumes we were called on root inode */
    assert(inode->flags & FS_DIR);
    assert(inode->tnode_list != NULL);
    assert(name != NULL);

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

struct file *initrd_open(struct inode *inode, int *error) {
    struct initrd_file_entry *entry = (struct initrd_file_entry*) inode->private_data;
    
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

ssize_t initrd_read(struct file *file, void *buffer, size_t _len) {
    if (file->flags & FS_DIR) {
        return -EISDIR;
    }

    size_t len = MIN(_len, file->length - file->position + 1);
    if (len <= 1) {
        return 0;
    }

    memcpy(buffer, (void*) (initrd_start + file->start + file->position), len - 1);
    ((char*) buffer)[len - 1] = '\0';
    file->position += len - 1;
    return (ssize_t) len;
}

int initrd_stat(struct inode *inode, struct stat *stat_struct) {
    stat_struct->st_size = inode->size;
    stat_struct->st_blocks = 1;
    stat_struct->st_blksize = stat_struct->st_size;
    stat_struct->st_ino = inode->index;
    stat_struct->st_dev = inode->device;
    stat_struct->st_mode = inode->mode;
    stat_struct->st_rdev = 0;
    return 0;
}

struct tnode *initrd_mount(struct file_system *current_fs, char *device_path) {
    struct vm_region *initrd = find_vm_region(VM_INITRD);
    assert(initrd != NULL);
    assert(strlen(device_path) == 0);

    initrd_start = initrd->start;
    num_files = *((int64_t*) initrd_start);
    file_list = (struct initrd_file_entry*) (initrd_start + sizeof(int64_t));

    struct inode *root = calloc(1, sizeof(struct inode));
    root->index = fs_get_next_inode_id();
    root->size = initrd->end - initrd->start;
    root->super_block = &super_block;
    root->flags = FS_DIR;
    root->device = 1;
    root->i_op = &initrd_dir_i_op;
    root->mode = S_IFDIR | 0777;
    init_spinlock(&root->lock);

    struct tnode *t_root = malloc(sizeof(struct tnode));
    t_root->inode = root;

    current_fs->super_block = &super_block;
    super_block.root = t_root;
    super_block.device = root->device;

    struct initrd_file_entry *entry = file_list;
    for (size_t i = 0; i < num_files; i++) {
        struct inode *inode = calloc(1, sizeof(struct inode));
        inode->index = fs_get_next_inode_id();
        inode->size = entry[i].length;
        inode->super_block = &super_block;
        inode->flags = FS_FILE;
        inode->device = root->device;
        inode->mode = S_IFREG | 0777;
        inode->i_op = &initrd_i_op;
        inode->private_data = entry + i;
        inode->parent = t_root;
        init_spinlock(&inode->lock);

        struct tnode *to_add = malloc(sizeof(struct tnode));
        to_add->name = entry[i].name;
        to_add->inode = inode;

        root->tnode_list = add_tnode(root->tnode_list, to_add);
    }

    return t_root;
}

void init_initrd() {
    load_fs(&fs);
}