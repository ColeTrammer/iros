#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>

#include <kernel/fs/vfs.h>
#include <kernel/fs/dev.h>
#include <kernel/fs/ext2.h>
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
    
    /* Shouldn't let you at a / at the end of a file name or root (but only if the path is // and not /./) */
    if ((path != NULL && path[0] == '/') && ((inode->flags & FS_FILE) || (inode == inode->parent->inode && strlen(_path) == 2))) {
        free(save_path);
        return NULL;
    }

    free(save_path);
    return parent;
}

int fs_create(const char *file_name, mode_t mode) {
    char *path = malloc(strlen(file_name) + 1);
    strcpy(path, file_name);

    char *last_slash = strrchr(path, '/');
    *last_slash = '\0';

    struct tnode *tparent;
    /* Root is a special case */
    if (last_slash == path) {
        tparent = iname("/");
    } else {
        tparent = iname(path);
    }

    if (tparent == NULL) {
        free(path);
        return -ENOENT;
    }

    struct mount *mount = tparent->inode->mounts;
    while (mount != NULL) {
        if (strcmp(mount->name, last_slash + 1) == 0) {
            free(path);
            return -EEXIST;
        }

        mount = mount->next;
    }

    tparent->inode->i_op->lookup(tparent->inode, NULL);
    if (!tparent->inode->tnode_list || find_tnode(tparent->inode->tnode_list, last_slash + 1) != NULL) {
        free(path);
        return -EEXIST;
    }

    if (!tparent->inode->i_op->create) {
        free(path);
        return -EINVAL;
    }

    debug_log("Adding to: [ %s ]\n", tparent->name);

    int error = 0;
    struct inode *inode = tparent->inode->i_op->create(tparent, last_slash + 1, mode | S_IFREG, &error);
    if (inode == NULL) {
        free(path);
        return error;
    }

    struct tnode *tnode = malloc(sizeof(struct tnode));
    tnode->inode = inode;
    tnode->name = malloc(strlen(last_slash + 1) + 1);
    strcpy(tnode->name, last_slash + 1);
    tparent->inode->tnode_list = add_tnode(tparent->inode->tnode_list, tnode);

    free(path);
    return 0;
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
        *error = -ENOENT;
        return NULL;
    }
    
    fs_inode_put(tnode->inode);

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
    struct inode *inode = fs_inode_get(file->device, file->inode_idenifier);
    if (!inode->tnode_list) {
        inode->i_op->lookup(inode, NULL);
        if (!inode->tnode_list) {
            return -EINVAL;
        }
    }

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
    if (len == 0) { return 0; }

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

off_t fs_seek(struct file *file, off_t offset, int whence) {
    off_t new_position;
    if (whence == SEEK_SET) {
        new_position = offset;
    } else if (whence == SEEK_CUR) {
        new_position = file->position + offset;
    } else if (whence == SEEK_END) {
        new_position = file->length + offset;
    } else {
        debug_log("Invalid arguments for fs_seek - whence: %d\n", whence);
        return -EINVAL;
    }

    if (new_position > file->length) {
        return -EINVAL;
    }

    file->position = new_position;
    return new_position;
}

long fs_tell(struct file *file) {
    return file->position;
}

void load_fs(struct file_system *fs) {
    assert(fs);

    fs->next = file_systems;
    file_systems = fs;
}

int fs_stat(const char *path, struct stat *stat_struct) {
    struct tnode *tnode = iname(path);
    if (tnode == NULL) {
        return -ENOENT;
    }

    struct inode *inode = tnode->inode; 
    if (!inode->i_op->stat) {
        return -EINVAL;
    }

    return inode->i_op->stat(inode, stat_struct);
}

int fs_ioctl(struct file *file, unsigned long request, void *argp) {
    struct inode *inode = fs_inode_get(file->device, file->inode_idenifier);

    if (inode->i_op->ioctl) {
        return inode->i_op->ioctl(inode, request, argp);
    }

    return -ENOTTY;
}

int fs_truncate(struct file *file, off_t length) {
    char *file_contents = malloc(length);
    ssize_t read = fs_read(file, file_contents, length);
    if (read < 0) {
        return (int) read;
    }

    /* Makes rest of file null bytes */
    if (read < length) {
        memset(file_contents + read, 0, length - read);
    }

    file->position = 0;
    ssize_t ret = fs_write(file, file_contents, length);
    file->position = 0;

    if (ret < 0) {
        return (int) ret;
    }

    return 0;
}

int fs_mkdir(const char *_path, mode_t mode) {
    char *path = malloc(strlen(_path) + 1);
    strcpy(path, _path);

    char *last_slash = strrchr(path, '/');
    *last_slash = '\0';

    struct tnode *tparent;
    /* Root is a special case */
    if (last_slash == path) {
        tparent = iname("/");
    } else {
        tparent = iname(path);
    }

    if (tparent == NULL) {
        free(path);
        return -ENOENT;
    }

    if (tparent->inode->flags & FS_FILE) {
        free(path);
        return -EINVAL;
    }

    struct mount *mount = tparent->inode->mounts;
    while (mount != NULL) {
        if (strcmp(mount->name, last_slash + 1) == 0) {
            free(path);
            return -EEXIST;
        }

        mount = mount->next;
    }

    tparent->inode->i_op->lookup(tparent->inode, NULL);
    if (!tparent->inode->tnode_list || find_tnode(tparent->inode->tnode_list, last_slash + 1) != NULL) {
        free(path);
        return -EEXIST;
    }

    if (!tparent->inode->i_op->mkdir) {
        free(path);
        return -EINVAL;
    }

    debug_log("Adding dir to: [ %s ]\n", tparent->name);

    int error = 0;
    struct inode *inode = tparent->inode->i_op->mkdir(tparent, last_slash + 1, mode | S_IFDIR, &error);
    if (inode == NULL) {
        free(path);
        return error;
    }

    struct tnode *tnode = malloc(sizeof(struct tnode));
    tnode->inode = inode;
    tnode->name = malloc(strlen(last_slash + 1) + 1);
    strcpy(tnode->name, last_slash + 1);
    tparent->inode->tnode_list = add_tnode(tparent->inode->tnode_list, tnode);

    free(path);
    return 0;
}

int fs_mount(const char *src, const char *path, const char *type) {
    debug_log("Mounting FS: [ %s, %s ]\n", type, path);

    struct file_system *file_system = file_systems;
    while (file_system != NULL) {
        if (strcmp(file_system->name, type) == 0) {
            struct mount *mount = malloc(sizeof(struct mount));
            if (strcmp(path, "/") == 0) {
                mount->name = "/";
                mount->next = NULL;
                char *dev_path = malloc(strlen(src) + 1);
                strcpy(dev_path, src);
                mount->device_path = dev_path;
                mount->fs = file_system;
                file_system->mount(file_system, mount->device_path);
                mount->super_block = file_system->super_block;
                mount->super_block->root->name = "/";
                mount->super_block->root->inode->parent = mount->super_block->root;

                /* For now, when mounting as / when there is already something mounted,
                   we will just move things around instead of unmounting what was
                   already there */
                if (root != NULL) {
                    mount->super_block->root->inode->mounts = root;
                    root->next = root->super_block->root->inode->mounts;
                    root->super_block->root->inode->mounts = NULL;

                    root->name = root->fs->name;
                    char *name = malloc(strlen(root->name) + 1);
                    strcpy(name, root->name);
                    root->super_block->root->name = name;

                    struct mount *mount_iter = mount->super_block->root->inode->mounts;
                    while (mount_iter != NULL) {
                        mount_iter->super_block->root->inode->parent = mount->super_block->root;
                        mount_iter = mount_iter->next;
                    }
                }

                root = mount;

                fs_inode_create_store(file_system->super_block->device);
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
            if (strlen(path_copy) == 0) {
                mount_on = root->super_block->root;
            } else {
                mount_on = iname(path_copy);
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
            char *dev_path = malloc(strlen(src) + 1);
            strcpy(dev_path, src);
            mount->device_path = dev_path;            
            mount->fs = file_system;
            mount->next = NULL;
            assert(file_system->mount(file_system, mount->device_path));
            mount->super_block = file_system->super_block;
            mount->super_block->root->name = name;
            mount->super_block->root->inode->parent = mount_on;

            fs_inode_create_store(file_system->super_block->device);
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
    init_fs_inode_store();

    init_initrd();
    init_dev();
    init_ext2();

    /* Mount INITRD as root */
    int error = fs_mount("", "/", "initrd");
    assert(error == 0);

    /* Mount dev at /dev */
    error = fs_mount("", "/dev", "dev");
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