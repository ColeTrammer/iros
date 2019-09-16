#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/param.h>
#include <errno.h>

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
    &initrd_lookup, &initrd_open
};

static struct file_operations initrd_f_op = {
    &initrd_close, &initrd_read, &initrd_write
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
        file->f_op = &initrd_f_op;
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

int initrd_close(struct file *file) {
    free(file);
    return 0;
}

ssize_t initrd_read(struct file *file, void *buffer, size_t _len) {
    if (file->flags & FS_DIR) {
        return -EISDIR;
    }

    size_t len = MIN(_len, file->length - (file->position - file->start));
    memcpy(buffer, (void*) (initrd_start + file->start + file->position), len);
    return (ssize_t) len;
}

ssize_t initrd_write(struct file *file, const void *buffer, size_t len) {
    (void) file;
    (void) buffer;
    (void) len;
    return -EINVAL;
}

struct tnode *initrd_mount(struct file_system *current_fs) {
    struct vm_region *initrd = find_vm_region(VM_INITRD);
    assert(initrd != NULL);
    
    initrd_start = initrd->start;
    num_files = *((int64_t*) initrd_start);
    file_list = (struct initrd_file_entry*) (initrd_start + sizeof(int64_t));

    struct inode *root = calloc(1, sizeof(struct inode));
    root->index = fs_get_next_inode_id();
    root->size = initrd->end - initrd->start;
    root->super_block = &super_block;
    root->flags = FS_DIR;
    root->device = 0;  /* Update when there is other devices... */
    root->i_op = &initrd_i_op;
    init_spinlock(&root->lock);

    struct tnode *t_root = malloc(sizeof(struct tnode));
    t_root->name = "/";
    t_root->inode = root;

    current_fs->super_block = &super_block;
    super_block.root = t_root;

    struct initrd_file_entry *entry = file_list;
    for (size_t i = 0; i < num_files; i++) {
        struct inode *inode = calloc(1, sizeof(struct inode));
        inode->index = fs_get_next_inode_id();
        inode->size = entry[i].length;
        inode->super_block = &super_block;
        inode->flags = FS_FILE;
        inode->device = 0;
        inode->i_op = &initrd_i_op;
        inode->private_data = entry + i;
        inode->parent = root;
        init_spinlock(&inode->lock);

        struct tnode *to_add = malloc(sizeof(struct tnode));
        to_add->name = entry[i].name;
        to_add->inode = inode;

        root->tnode_list = add_tnode(root->tnode_list, to_add);
    }

    debug_log("INITRD Mounted: [ %llu, %#.16lX ]\n", num_files, initrd_start);
    return t_root;
}

void init_initrd() {
    load_fs(&fs);
}