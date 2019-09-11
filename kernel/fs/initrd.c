#include <stdio.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <string.h>

#include <kernel/fs/file.h>
#include <kernel/fs/initrd.h>
#include <kernel/fs/file_system.h>
#include <kernel/fs/vfs.h>
#include <kernel/fs/super_block.h>
#include <kernel/mem/vm_region.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/hal/output.h>

static struct file_system fs;
static struct super_block super_block;

/* Should be allocated in super block */
static int64_t num_files;
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

struct inode *initrd_lookup(struct inode *inode, const char *name) {
    /* Assumes we were called on root inode */
    assert(strcmp(inode->name, "/") == 0);
    assert(inode->flags & FS_DIR);

    struct initrd_file_entry *entry = file_list;
    for (int64_t i = 0; i < num_files; i++) {
        if (strcmp(name, entry[i].name) == 0) {
            struct inode *inode = calloc(1, sizeof(struct inode));
            inode->name = name;
            inode->flags = FS_FILE;
            inode->size = entry[i].length;
            inode->i_op = &initrd_i_op;
            inode->super_block = &super_block;
            inode->private_data = entry + i;
            return inode;
        }
    }

    return NULL;
}

struct file *initrd_open(struct inode *inode) {
    struct initrd_file_entry *entry = (struct initrd_file_entry*) inode->private_data;
    assert(entry != NULL);

    struct file *file = calloc(sizeof(struct file), 1);
    file->name = inode->name;
    file->length = entry->length;
    file->start = entry->offset;
    file->position = 0;
    file->f_op = &initrd_f_op;
    file->device = 0; /* Update when there is other devices... */
    return file;
}

void initrd_close(struct file *file) {
    free(file);
}

void initrd_read(struct file *file, void *buffer, size_t len) {
    memcpy(buffer, (void*) (initrd_start + file->start + file->position), len);
}

void initrd_write(struct file *file, const void *buffer, size_t len) {
    (void) file;
    printf("Can't write to initrd.\nBuffer: %#.16lX | Len: %lu\n", (uintptr_t) buffer, len);
    abort();
}

struct inode *initrd_mount(struct file_system *fs) {
    struct vm_region *initrd = find_vm_region(VM_INITRD);
    assert(initrd != NULL);
    
    initrd_start = initrd->start;
    num_files = *((int64_t*) initrd_start);
    file_list = (struct initrd_file_entry*) (initrd_start + sizeof(int64_t));

    struct inode *root = calloc(1, sizeof(struct inode));
    root->name = "/";
    root->size = initrd->end - initrd->start;
    root->super_block = &super_block;
    root->flags = FS_DIR;
    root->device = 0;
    root->i_op = &initrd_i_op;

    fs->super_block = &super_block;
    super_block.root = root;

    debug_log("INITRD Mounted: [ %ld, %#.16lX ]\n", num_files, initrd_start);
    return root;
}

void init_initrd() {
    load_fs(&fs);
}