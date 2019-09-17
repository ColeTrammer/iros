#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>

#include <kernel/fs/vfs.h>
#include <kernel/fs/dev.h>
#include <kernel/fs/inode.h>
#include <kernel/fs/inode_store.h>
#include <kernel/fs/initrd.h>
#include <kernel/fs/file_system.h>
#include <kernel/hal/output.h>

static struct file_system *file_systems;
static struct mount *root;

struct tnode *iname(const char *_path) {
    assert(root != NULL);
    assert(root->super_block != NULL);

    struct tnode *t_root = root->super_block->root;
    if (t_root == NULL) {
        return NULL;
    }

    assert(t_root->inode != NULL);
    assert(t_root->inode->i_op != NULL);
    assert(_path[0] == '/');

    char *path = malloc(strlen(_path) + 1);
    char *save_path = path;
    strcpy(path, _path);

    struct tnode *parent = t_root;
    
    /* Main VFS Loop */
    char *last_slash = strchr(path + 1, '/');
    while (parent != NULL && path != NULL && path[1] != '\0') {
        /* Exit if we're trying to lookup past a file */
        if (!(parent->inode->flags & FS_DIR)) {
            free(save_path);
            return NULL;
        }

        /* Check for . and .. */
        if (path[1] == '.' && (path[2] == '/' || path[2] == '\0')) {
            /* Dot means current directory so just skip processing */
            goto vfs_loop_end;
        }

        if (path[1] == '.' && path[2] == '.' && (path[3] == '/' || path[3] == '\0')) {
            /* Simply go to parent */
            parent = parent->inode->parent;
            goto vfs_loop_end;
        }

        /* Ensures passed name will be correct */
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
    
    /* Shouldn't let you at a / at the end of a file name or root */
    if ((path != NULL && path[0] == '/') && ((inode->flags & FS_FILE) || (inode == inode->parent->inode))) {
        free(save_path);
        return NULL;
    }

    free(save_path);
    return parent;
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

    struct tnode *tnode = iname(file_name);
    if (tnode == NULL) {
        debug_log("Tnode not found\n");
        *error = -ENOENT;
        return NULL;
    }
    
    if (!fs_inode_get(tnode->inode->index)) {
        fs_inode_put(tnode->inode);
    }

    return tnode->inode->i_op->open(tnode->inode, error);
}

/* Should potentially remove inode from inode_store */
int fs_close(struct file *file) {
    int error = 0;
    if (file->f_op->close) {
        error = file->f_op->close(file);
    }

    free(file);
    return error;
}

/* Default dir read: works for file systems completely cached in memory */
static ssize_t default_dir_read(struct file *file, void *buffer, size_t len) {
    if (len != sizeof(struct dirent)) {
        return -EINVAL;
    }

    struct dirent *entry = (struct dirent*) buffer;
    struct inode *inode = fs_inode_get(file->inode_idenifier);
    struct tnode *tnode = find_tnode_index(inode->tnode_list, file->position);
    if (!tnode) {
        /* Traverse mount points as well */
        size_t len = get_tnode_list_length(inode->tnode_list);
        size_t mount_index = file->position - len;
        size_t i = 0;
        struct mount *mount = inode->mounts;
        while (mount != NULL) {
            if (i++ == mount_index) {
                file->position++;

                entry->d_ino = mount->super_block->root->inode->index;
                strcpy(entry->d_name, mount->name);
                return (ssize_t) len;
            }

            mount = mount->next;
        }

        /* Should sent an error that indicates there's no more to read (like EOF) */
        return -EINVAL;
    }

    file->position++;

    entry->d_ino = tnode->inode->index;
    strcpy(entry->d_name, tnode->name);
    return (ssize_t) len;
}

ssize_t fs_read(struct file *file, void *buffer, size_t len) {
    if (file->f_op->read) {
        return file->f_op->read(file, buffer, len);
    }

    if (file->flags & FS_DIR) {
        return default_dir_read(file, buffer, len);
    }

    return -EINVAL;
}

ssize_t fs_write(struct file *file, const void *buffer, size_t len) {
    if (file->f_op->write) {
        return file->f_op->write(file, buffer, len);
    }

    return -EINVAL;
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
                mount->super_block->root->name = "/";
                mount->super_block->root->inode->parent = mount->super_block->root;
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

            struct tnode *mount_on;
            
            /* Means we are mounting to root */
            if (parent_end == path_copy) {
                mount_on = root->super_block->root;
            } else {
                mount_on = iname(path);
            }
            
            if (mount_on == NULL || !(mount_on->inode->flags & FS_DIR)) {
                free(path_copy);
                return -1;
            }

            struct mount **list = &mount_on->inode->mounts;
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
            mount->super_block->root->name = name;
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
    if (relative_path[0] == '/') {
        char *path = malloc(strlen(relative_path) + 1);
        strcpy(path, relative_path);
        return path;
    }

    if (cwd[0] == '/' && cwd[1] == '\0') {
        size_t len = strlen(relative_path) + 2;
        char *path = malloc(len);
        path[0] = '/';
        strcpy(path + 1, relative_path);
        return path;
    }

    size_t len = strlen(cwd) + strlen(relative_path) + 2;
    char *path = malloc(len);
    strcpy(path, cwd);
    strcat(path, "/");
    strcat(path, relative_path);

    return path;
}

char *get_tnode_path(struct tnode *tnode) {
    /* Check if root */
    if (tnode == tnode->inode->parent) {
        char *ret = malloc(2);
        strcpy(ret, "/");
        return ret;
    }

    ssize_t size = 15;
    char **name_buffer = malloc(size * sizeof(char**));

    ssize_t len = 1;
    ssize_t i;
    for (i = 0; tnode->inode->parent != tnode; i++) {
        name_buffer[i] = tnode->name;
        len += strlen(tnode->name) + 1;
        tnode = tnode->inode->parent;

        if (i >= size) {
            size *= 2;
            name_buffer = realloc(name_buffer, size);
        }
    }

    char *ret = malloc(len);
    ret[0] = '\0';
    for (i--; i>= 0; i--) {
        strcat(ret, "/");
        strcat(ret, name_buffer[i]);
    }

    free(name_buffer);
    return ret;
}