#include <sys/param.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

#include <kernel/fs/vfs.h>
#include <kernel/fs/dev.h>
#include <kernel/fs/inode.h>
#include <kernel/fs/inode_store.h>
#include <kernel/fs/initrd.h>
#include <kernel/fs/file_system.h>
#include <kernel/hal/output.h>

static struct file_system *file_systems;
static struct mount *root;

static struct inode *iname(const char *path, int *error) {
    assert(root != NULL);
    assert(root->super_block != NULL);

    struct tnode *t_root = root->super_block->root;
    if (t_root == NULL) {
        *error = -ENOENT;
        return NULL;
    }

    assert(t_root->inode != NULL);
    assert(t_root->inode->i_op != NULL);
    
    struct tnode *tnode = t_root->inode->i_op->lookup(t_root->inode, path + 1);
    if (tnode == NULL) {
        *error = -ENOENT;
        return NULL;
    }

    struct inode *inode = tnode->inode;
    return inode;
}

void init_vfs() {
    init_initrd();
    init_dev();

    /* Mount INITRD as root */
    int error = fs_mount("initrd", "/", 0);
    assert(error == 0);

    /* Mount dev at /dev */
    error = fs_mount("dev", "/dev", 0);
    assert(error == 0);
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

    struct inode *inode = iname(file_name, error);
    if (*error < 0) {
        return NULL;
    }
    
    if (!fs_inode_get(inode->index)) {
        fs_inode_put(inode);
    }

    *error = 0;
    return inode->i_op->open(inode);
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

int fs_mount(const char *type, const char *path, dev_t device) {
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
                return 0;
            }

            char *path_copy = malloc(strlen(path) + 1);
            strcpy(path_copy, path);

            /* Needs to find parent of the path, so we can mount the fs on it */
            if (path_copy[strlen(path_copy) - 1] == '/') {
                path_copy[strlen(path_copy) - 1] = '\0';
            }

            char *parent_end = strrchr(path_copy, '/');
            *parent_end = '\0';

            int error = 0;
            struct inode *mount_on;
            
            /* Means we are mounting to root */
            if (parent_end == path_copy) {
                mount_on = root->super_block->root->inode;
            } else {
                mount_on = iname(path, &error);
            }
            
            if (mount_on == NULL) {
                free(path_copy);
                return error;
            }

            struct mount **list = &mount_on->mounts;
            while (*list != NULL) {
                list = &(*list)->next;
            }
            *list = mount;

            char *name = malloc(strlen(parent_end + 1) + 1);
            strcpy(name, parent_end + 1);

            mount->name = name;
            mount->device = device;
            mount->next = NULL;
            file_system->mount(file_system);
            mount->super_block = file_systems->super_block;

            free(path_copy);
            return 0;
        }

        file_system = file_system->next;
    }

    /* Should instead error because fs type is not found */
    assert(false);
    return -1;
}