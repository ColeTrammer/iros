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

struct inode *iname(const char *_path) {
    assert(root != NULL);
    assert(root->super_block != NULL);

    struct tnode *t_root = root->super_block->root;
    if (t_root == NULL) {
        return NULL;
    }

    assert(t_root->inode != NULL);
    assert(t_root->inode->i_op != NULL);
    
    /* Means we are on root */
    if (_path[1] == '\0') {
        return t_root->inode;
    }

    char *path = malloc(strlen(_path) + 1);
    char *save_path = path;
    strcpy(path, _path);

    struct tnode *parent = t_root;
    
    /* Main VFS Loop */
    char *last_slash = strpbrk(path + 1, "/");
    while (parent != NULL && path != NULL && path[1] != '\0') {
        /* Exit if we're trying to lookup past a file */
        if (!(parent->inode->flags & FS_DIR)) {
            free(save_path);
            return NULL;
        }

        /* Ensures passed name will be corrent */
        if (last_slash != NULL) {
            *last_slash = '\0';
        }

        /* Checks mounts first so that we don't do uneccessary disk IO */
        struct mount *mount = parent->inode->mounts;
        while (mount != NULL) {
            assert(mount->name);
            assert(mount->super_block);
            assert(mount->super_block->root);
            assert(mount->super_block->root->inode);

            if (strcmp(mount->name, path + 1) == 0) {
                parent = mount->super_block->root;
                goto vfs_loop_end;
            }

            mount = mount->next;
        }

        /* Check using lookup */
        assert(parent->inode);
        assert(parent->inode->i_op);
        assert(parent->inode->i_op->lookup);
        
        parent = parent->inode->i_op->lookup(parent->inode, path + 1);

    vfs_loop_end:
        path = last_slash;
        if (path != NULL) {
            path[0] = '/';
            last_slash = strchr(path + 1, '/');
        }
    }
    
    if (parent == NULL) {
        /* Couldn't find what we were looking for */
        free(save_path);
        return NULL;
    }

    struct inode *inode = parent->inode;
    
    /* Shouldn't let you at a / at the end of a file name */
    if ((path != NULL && path[0] == '/') && inode->flags & FS_FILE) {
        free(save_path);
        return NULL;
    }

    free(save_path);
    return inode;
}

struct file *fs_open(const char *file_name, int *error) {
    if (file_name == NULL) {
        *error = -EINVAL;
        return NULL;
    }

    /* We Don't Support Not Root Paths */
    if (file_name[0] != '/') {
        debug_log("Invalid path\n");
        *error = -ENOENT;
        return NULL;
    }

    struct inode *inode = iname(file_name);
    if (inode == NULL) {
        debug_log("Inode not found\n");
        *error = -ENOENT;
        return NULL;
    }
    
    if (!fs_inode_get(inode->index)) {
        fs_inode_put(inode);
    }

    return inode->i_op->open(inode, error);
}

int fs_close(struct file *file) {
    return file->f_op->close(file);
}

ssize_t fs_read(struct file *file, void *buffer, size_t len) {
    return file->f_op->read(file, buffer, len);
}

ssize_t fs_write(struct file *file, const void *buffer, size_t len) {
    return file->f_op->write(file, buffer, len);
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
                mount->super_block->root->inode->parent = mount->super_block->root->inode;
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

            struct inode *mount_on;
            
            /* Means we are mounting to root */
            if (parent_end == path_copy) {
                mount_on = root->super_block->root->inode;
            } else {
                mount_on = iname(path);
            }
            
            if (mount_on == NULL || !(mount_on->flags & FS_DIR)) {
                free(path_copy);
                return -1;
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
            mount->super_block->root->inode->parent = mount_on;

            free(path_copy);
            return 0;
        }

        file_system = file_system->next;
    }

    /* Should instead error because fs type is not found */
    assert(false);
    return -1;
}

struct file *fs_clone(struct file *file) {
    struct file *new_file = malloc(sizeof(struct file));
    memcpy(new_file, file, sizeof(struct file));
    return new_file;
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

char *get_full_path(char *cwd, const char *relative_path) {
    size_t offset = relative_path[0] == '.' ? 1 : 0;
    size_t len = strlen(cwd) + strlen(relative_path + offset) + 1;
    char *path = malloc(len);
    strcpy(path, cwd);
    strcat(path, relative_path + offset);
    return path;
}