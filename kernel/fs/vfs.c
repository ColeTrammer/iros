#include <sys/param.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include <kernel/fs/vfs.h>
#include <kernel/fs/inode.h>
#include <kernel/fs/inode_store.h>
#include <kernel/fs/initrd.h>
#include <kernel/fs/file_system.h>
#include <kernel/hal/output.h>

static struct file_system *file_systems;
static struct mount *root;

void init_vfs() {
    init_initrd();

    /* Mount INITRD as root */
    fs_mount("initrd", "/", 0);
}

struct file *fs_open(const char *file_name, int *error) {
    if (file_name == NULL) {
        *error = -EINVAL;
        return NULL;
    }

    /* We Don't Support Not Root Paths */
    if (file_name[0] != '/') {
        *error = -ENOENT;
        return NULL;
    }

    assert(root != NULL);
    assert(root->super_block != NULL);

    struct tnode *t_root = root->super_block->root;
    if (t_root == NULL) {
        *error = -ENOENT;
        return NULL;
    }

    assert(t_root->inode != NULL);
    assert(t_root->inode->i_op != NULL);
    
    struct tnode *tnode = t_root->inode->i_op->lookup(t_root->inode, file_name + 1);
    if (tnode == NULL) {
        *error = -ENOENT;
        return NULL;
    }

    debug_log("File Opened: [ %s ]\n", file_name);
    struct inode *inode = tnode->inode;
    fs_inode_put(inode);
    
    *error = 0;
    return inode->i_op->open(tnode->inode);
}

void fs_close(struct file *file) {
    file->f_op->close(file);
}

void fs_read(struct file *file, void *buffer, size_t len) {
    file->f_op->read(file, buffer, MIN(len, file->length - (file->position - file->start)));
}

void fs_write(struct file *file, const void *buffer, size_t len) {
    file->f_op->write(file, buffer, len);
}

int fs_seek(struct file *file, off_t offset, int whence) {
    off_t new_position;
    if (whence == SEEK_SET) {
        new_position = offset;
    } else if (whence == SEEK_CUR) {
        new_position = file->position + offset;
    } else if (whence == SEEK_END) {
        new_position = file->length + offset;
    } else {
        printf("Invalid arguments for fs_seek - whence: %d\n", whence);
        abort();
    }

    if (new_position > file->length) {
        return -1;
    }

    file->position = new_position;
    return 0;
}

long fs_tell(struct file *file) {
    return file->position;
}

void load_fs(struct file_system *fs) {
    fs->next = file_systems;
    file_systems = fs;
}

void fs_mount(const char *type, const char *path, dev_t device) {
    struct file_system *file_system = file_systems;
    while (file_system != NULL) {
        if (strcmp(file_system->name, type) == 0) {
            struct mount *mount = malloc(sizeof(struct mount));
            if (strcmp(path, "/") == 0) {
                mount->name = "/";
                mount->next = NULL;
                mount->device = device;
                file_system->mount(file_system);
                mount->super_block = file_system->super_block;
                root = mount;
                return;
            }

            /* Should handle non root mounts */
            assert(false);
        }

        file_system = file_system->next;
    }

    /* Should instead error because fs type is not found */
    assert(false);
}