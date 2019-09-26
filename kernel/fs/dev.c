#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include <kernel/fs/file.h>
#include <kernel/fs/inode.h>
#include <kernel/fs/inode_store.h>
#include <kernel/fs/dev.h>
#include <kernel/fs/file_system.h>
#include <kernel/fs/vfs.h>
#include <kernel/fs/super_block.h>
#include <kernel/mem/vm_region.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/hal/output.h>
#include <kernel/util/spinlock.h>

static struct file_system fs;
static struct super_block super_block;

static struct file_system fs = {
    "dev", 0, &dev_mount, NULL, NULL
};

static struct inode_operations dev_i_op = {
    &dev_lookup, &dev_open, &dev_stat
};

static struct inode_operations dev_dir_i_op = {
    &dev_lookup, &dev_open, &dev_stat
};

static struct file_operations dev_f_op = {
    &dev_close, &dev_read, &dev_write
};

static struct file_operations dev_dir_f_op = {
    NULL, NULL, NULL
};

struct tnode *dev_lookup(struct inode *inode, const char *name) {
    assert(name != NULL);

    if (inode->tnode_list == NULL) {
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

struct file *dev_open(struct inode *inode, int *error) {
    struct file *file = calloc(sizeof(struct file), 1);
    file->inode_idenifier = inode->index;
    file->length = inode->size;
    file->start = 0;
    file->position = 0;
    file->f_op = inode->flags & FS_FILE ? &dev_f_op : &dev_dir_f_op;
    file->device = inode->device;
    file->flags = inode->flags;

    if (inode->private_data && ((struct device*) inode->private_data)->ops->open) {
        *error = ((struct device*) inode->private_data)->ops->open(inode->private_data, file);
    }

    return file;
}

int dev_close(struct file *file) {
    struct inode *inode = fs_inode_get(file->inode_idenifier);
    assert(inode);

    int error = 0;
    if (((struct device*) inode->private_data)->ops->close) {
        error = ((struct device*) inode->private_data)->ops->close(inode->private_data);
    }

    return error;
}

ssize_t dev_read(struct file *file, void *buffer, size_t len) {
    if (file->flags & FS_DIR) {
        return -EISDIR;
    }

    struct inode *inode = fs_inode_get(file->inode_idenifier);
    assert(inode);

    if (((struct device*) inode->private_data)->ops->read) {
        return ((struct device*) inode->private_data)->ops->read(inode->private_data, file, buffer, len);
    }

    return -EINVAL;
}

ssize_t dev_write(struct file *file, const void *buffer, size_t len) {
    struct inode *inode = fs_inode_get(file->inode_idenifier);
    assert(inode);

    if (((struct device*) inode->private_data)->ops->write) {
        return ((struct device*) inode->private_data)->ops->write(inode->private_data, file, buffer, len);
    }

    return -EINVAL;
}

int dev_stat(struct inode *inode, struct stat *stat_struct) {
    stat_struct->st_size = inode->size;
    stat_struct->st_blocks = 0;
    stat_struct->st_blksize = stat_struct->st_size;
    stat_struct->st_ino = inode->index;
    stat_struct->st_dev = inode->device;
    stat_struct->st_mode = inode->mode; 
    stat_struct->st_rdev = (inode->flags & FS_FILE) ? ((struct device*) inode->private_data)->device_number : 0;
    return 0;
}

struct tnode *dev_mount(struct file_system *current_fs, char *device_path) {
    assert(current_fs != NULL);
    assert(strlen(device_path) == 0);

    struct tnode *t_root = calloc(1, sizeof(struct tnode));
    struct inode *root = calloc(1, sizeof(struct inode));

    t_root->inode = root;

    root->device = 2;
    root->flags = FS_DIR;
    root->i_op = &dev_dir_i_op;
    root->index = fs_get_next_inode_id();
    init_spinlock(&root->lock);
    root->mode = S_IFDIR | 0777;
    root->mounts = NULL;
    root->private_data = NULL;
    root->size = 0;
    root->super_block = &super_block;
    root->tnode_list = NULL;

    super_block.device = root->device;
    super_block.op = NULL;
    super_block.root = t_root;

    current_fs->super_block = &super_block;

    return t_root;
}

static char *to_dev_path(const char *path) {
    char *new_path = malloc(strlen(path) + 6);
    strcpy(new_path, "/dev/");
    strcat(new_path, path);
    return new_path;
}

void dev_add(struct device *device, const char *_path) {
    char *path = to_dev_path(_path);
    char *_name = strrchr(path, '/');
    *_name = '\0';
    _name++;

    struct tnode *parent = iname(path);
    if (parent == NULL) {
        /* Probably should add the directory that is missing */
        free(path);
        return;
    }

    /* Adds the device */
    if (device->ops->add) {
        device->ops->add(device);
    }

    struct inode *to_add = calloc(1, sizeof(struct inode));
    to_add->device = super_block.device;
    to_add->flags = FS_FILE;
    to_add->i_op = &dev_i_op;
    to_add->index = fs_get_next_inode_id();
    init_spinlock(&to_add->lock);
    to_add->mode = device->type | 0777;
    to_add->mounts = NULL;
    to_add->parent = parent;
    to_add->private_data = device;
    to_add->size = 0;
    to_add->super_block = &super_block;
    to_add->tnode_list = NULL;

    struct tnode *tnode = malloc(sizeof(struct tnode));
    
    char *name = malloc(strlen(_name) + 1);
    strcpy(name, _name);
    tnode->name = name;
    tnode->inode = to_add;

    parent->inode->tnode_list = add_tnode(parent->inode->tnode_list, tnode);

    free(path);
}

void dev_remove(const char *_path) {
    char *path = to_dev_path(_path);

    struct tnode *tnode = iname(path);
    if (tnode == NULL) {
        /* This probably shouldn't happen */
        free(path);
        return;
    }

    /* Frees the device */
    if (((struct device*) tnode->inode->private_data)->ops->remove) {
        ((struct device*) tnode->inode->private_data)->ops->remove(tnode->inode->private_data);
    }
    free(tnode->inode->private_data);

    /* Removes tnode */
    tnode->inode->parent->inode->tnode_list = remove_tnode(tnode->inode->parent->inode->tnode_list, tnode);
    free(tnode->inode);
    free(tnode->name);
    free(tnode);

    free(path);
}

void init_dev() {
    load_fs(&fs);
}