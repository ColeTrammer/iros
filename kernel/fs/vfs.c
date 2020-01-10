#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <kernel/fs/dev.h>
#include <kernel/fs/ext2.h>
#include <kernel/fs/file_system.h>
#include <kernel/fs/initrd.h>
#include <kernel/fs/inode.h>
#include <kernel/fs/inode_store.h>
#include <kernel/fs/pipe.h>
#include <kernel/fs/tmp.h>
#include <kernel/fs/vfs.h>
#include <kernel/hal/output.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/net/socket.h>
#include <kernel/proc/task.h>

#define INODE_REF_COUNT_DEBUG

static struct file_system *file_systems;
static struct mount *root;

static void bump_inode_reference(struct inode *inode) {
    spin_lock(&inode->lock);

#ifdef INODE_REF_COUNT_DEBUG
    debug_log("Ref count: [ %lu, %llu, %d ]\n", inode->device, inode->index, inode->ref_count + 1);
#endif /* INODE_REF_COUNT_DEBUG */

    assert(inode->ref_count > 0);
    inode->ref_count++;
    spin_unlock(&inode->lock);
}

void drop_inode_reference_unlocked(struct inode *inode) {
#ifdef INODE_REF_COUNT_DEBUG
    debug_log("Ref count: [ %lu, %llu, %d ]\n", inode->device, inode->index, inode->ref_count - 1);
#endif /* INODE_REF_COUNT_DEBUG */

    // Only delete inode if it's refcount is zero
    assert(inode->ref_count > 0);
    inode->ref_count--;
    if (inode->ref_count <= 0) {
        debug_log("Destroying inode: [ %lu, %llu ]\n", inode->device, inode->index);
        if (inode->i_op->on_inode_destruction) {
            inode->i_op->on_inode_destruction(inode);
        }

        fs_inode_del(inode->device, inode->index);

        spin_unlock(&inode->lock);

        free(inode);
        return;
    }

    spin_unlock(&inode->lock);
}

void drop_inode_reference(struct inode *inode) {
    spin_lock(&inode->lock);
    drop_inode_reference_unlocked(inode);
}

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
            /* Dot means current directory so just skip tasking */
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
    mode &= ~get_current_task()->process->umask;

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
    if (tparent->inode->tnode_list && find_tnode(tparent->inode->tnode_list, last_slash + 1) != NULL) {
        free(path);
        return -EEXIST;
    }

    if (!tparent->inode->i_op->create) {
        free(path);
        return -EINVAL;
    }

    debug_log("Adding to: [ %s ]\n", tparent->name);

    int error = 0;
    struct inode *inode = tparent->inode->i_op->create(tparent, last_slash + 1, mode, &error);
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

struct file *fs_open(const char *file_name, int flags, int *error) {
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

    bump_inode_reference(tnode->inode);

    struct file *file = tnode->inode->i_op->open(tnode->inode, flags, error);
    if (file == NULL) {
        return file;
    }

    init_spinlock(&file->lock);
    file->ref_count = 1;
    file->abilities = 0;
    file->open_flags = flags;
    if (flags & O_RDWR) {
        file->abilities |= FS_FILE_CAN_WRITE | FS_FILE_CAN_READ;
    } else if (flags & O_RDONLY) {
        file->abilities |= FS_FILE_CAN_READ;
    } else if (flags & O_WRONLY) {
        file->abilities |= FS_FILE_CAN_WRITE;
    }

    return file;
}

/* Should potentially remove inode from inode_store */
int fs_close(struct file *file) {
    assert(file);

    struct inode *inode = fs_inode_get(file->device, file->inode_idenifier);

    spin_lock(&file->lock);
    assert(file->ref_count > 0);
    file->ref_count--;
    if (file->ref_count <= 0) {
        spin_unlock(&file->lock);

        if (inode) {
            drop_inode_reference(inode);
        }

        int error = 0;
        if (file->f_op->close) {
            error = file->f_op->close(file);
        }

        free(file);
        return error;
    }

    spin_unlock(&file->lock);

    return 0;
}

/* Default dir read: works for file systems completely cached in memory */
static ssize_t default_dir_read(struct file *file, void *buffer, size_t len) {
    if (len != sizeof(struct dirent)) {
        return -EINVAL;
    }

    struct dirent *entry = (struct dirent *) buffer;
    struct inode *inode = fs_inode_get(file->device, file->inode_idenifier);
    if (!inode->tnode_list) {
        inode->i_op->lookup(inode, NULL);
        if (!inode->tnode_list) {
            return -EINVAL;
        }
    }

    spin_lock(&inode->lock);
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

                spin_unlock(&inode->lock);
                return (ssize_t) len;
            }

            mount = mount->next;
        }

        spin_unlock(&inode->lock);
        return 0;
    }

    file->position++;

    entry->d_ino = tnode->inode->index;
    strcpy(entry->d_name, tnode->name);

    spin_unlock(&inode->lock);

    return (ssize_t) len;
}

ssize_t fs_read(struct file *file, void *buffer, size_t len) {
    if (len == 0) {
        return 0;
    }

    assert(file);
    assert(file->f_op);
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

    debug_log("Loading fs: [ %s ]\n", fs->name);

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
    assert(file);

    char *file_contents = malloc(length);
    assert(file_contents);

    ssize_t read = fs_read(file, file_contents, length);
    if (read < 0) {
        return read;
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
    mode &= ~get_current_task()->process->umask;

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

int fs_create_pipe(struct file *pipe_files[2]) {
    struct inode *pipe_inode = pipe_new_inode();
    fs_inode_put(pipe_inode);
    int error = 0;
    pipe_files[0] = pipe_inode->i_op->open(pipe_inode, O_RDONLY, &error);
    pipe_files[0]->abilities |= FS_FILE_CAN_READ;
    init_spinlock(&pipe_files[0]->lock);
    pipe_files[0]->ref_count = 1;
    if (error != 0) {
        return error;
    }

    pipe_files[1] = pipe_inode->i_op->open(pipe_inode, O_WRONLY, &error);
    pipe_files[1]->abilities |= FS_FILE_CAN_WRITE;
    init_spinlock(&pipe_files[1]->lock);
    pipe_files[1]->ref_count = 1;
    if (error != 0) {
        fs_close(pipe_files[0]);
        return error;
    }

    return 0;
}

int fs_unlink(const char *path) {
    assert(path);

    debug_log("Unlinking: [ %s ]\n", path);

    struct tnode *tnode = iname(path);
    if (tnode == NULL) {
        return -ENOENT;
    }

    // Can't remove a unix socket that is being currently being used
    if (tnode->inode->socket_id != 0) {
        return -EBUSY;
    }

    if (tnode->inode->flags & FS_DIR) {
        debug_log("Name: [ %s, %u ]\n", tnode->name, tnode->inode->flags);
        return -EISDIR;
    }

    if (tnode->inode->i_op->unlink == NULL) {
        return -EINVAL;
    }

    spin_lock(&tnode->inode->lock);

    int ret = tnode->inode->i_op->unlink(tnode);
    if (ret != 0) {
        return ret;
    }

    tnode->inode->parent->inode->tnode_list = remove_tnode(tnode->inode->parent->inode->tnode_list, tnode);
    struct inode *inode = tnode->inode;
    free(tnode);

    drop_inode_reference_unlocked(inode);
    return 0;
}

static bool dir_empty(struct inode *inode) {
    assert(inode);
    assert(inode->flags & FS_DIR);
    if (inode->mounts != NULL) {
        /* Should send EBUSY not ENOTEMPTY */
        return false;
    }

    if (inode->tnode_list == NULL) {
        inode->i_op->lookup(inode, NULL);
        /* There is no dir entries */
        if (inode->tnode_list == NULL) {
            return true;
        }
    }

    size_t tnode_list_length = get_tnode_list_length(inode->tnode_list);
    if (tnode_list_length == 0) {
        return true;
    }

    /* Handle . and .. */
    if (tnode_list_length == 2) {
        return find_tnode(inode->tnode_list, ".") != NULL && find_tnode(inode->tnode_list, "..") != NULL;
    }

    return false;
}

int fs_rmdir(const char *path) {
    assert(path);

    struct tnode *tnode = iname(path);
    if (tnode == NULL) {
        return -ENOENT;
    }

    if (!(tnode->inode->flags & FS_DIR)) {
        return -ENOTDIR;
    }

    if (!dir_empty(tnode->inode)) {
        return -ENOTEMPTY;
    }

    if (tnode->inode->i_op->rmdir == NULL) {
        return -EINVAL;
    }

    spin_lock(&tnode->inode->lock);

    int ret = tnode->inode->i_op->rmdir(tnode);
    if (ret != 0) {
        return ret;
    }

    struct inode *inode = tnode->inode;
    inode->parent->inode->tnode_list = remove_tnode(inode->parent->inode->tnode_list, tnode);
    free(tnode);

    drop_inode_reference(inode);
    return 0;
}

int fs_chmod(const char *path, mode_t mode) {
    assert(path);

    struct tnode *tnode = iname(path);

    if (tnode == NULL) {
        return -ENOENT;
    }

    if (!tnode->inode->i_op->chmod) {
        return -EPERM;
    }

    // Don't yet support SETUID and SETGID
    mode &= 0777;

    // Retain type information
    mode |= tnode->inode->mode & ~07777;

    return tnode->inode->i_op->chmod(tnode->inode, mode);
}

intptr_t fs_mmap(void *addr, size_t len, int prot, int flags, struct file *file, off_t offset) {
    if (file == NULL) {
        return -EINVAL;
    }

    struct inode *inode = fs_inode_get(file->device, file->inode_idenifier);
    assert(inode);

    if (inode->i_op->mmap) {
        bump_inode_reference(inode);
        return inode->i_op->mmap(addr, len, prot, flags, inode, offset);
    }

    return -ENODEV;
}

int fs_munmap(void *addr, size_t len) {
    (void) len;

    debug_log("Doing fs_munmap: [ %#.16lX ]\n", (uintptr_t) addr);

    struct vm_region *region = find_vm_region_by_addr((uintptr_t) addr);
    if (region == NULL) {
        debug_log("Didn't find a corresponding region\n");
        return -EINVAL;
    }

    struct inode *inode = region->backing_inode;
    assert(inode);

    drop_inode_reference(inode);
    return 0;
}

int fs_rename(char *old_path, char *new_path) {
    assert(old_path);
    assert(new_path);

    debug_log("Rename: [ %s, %s ]\n", old_path, new_path);

    struct tnode *old = iname(old_path);
    if (old == NULL) {
        return -ENOENT;
    }

    char *new_path_last_slash = strrchr(new_path, '/');
    assert(new_path_last_slash);

    struct tnode *new_parent;
    if (new_path_last_slash == new_path) {
        new_parent = root->super_block->root;
    } else {
        *new_path_last_slash = '\0';
        new_parent = iname(new_path);
        *new_path_last_slash = '/';
    }

    if (new_parent == NULL) {
        return -ENOENT;
    }

    if (old->inode->super_block != new_parent->inode->super_block) {
        return -EXDEV;
    }

    struct tnode *existing_tnode = iname(new_path);
    if (existing_tnode && (((existing_tnode->inode->flags & FS_DIR) && !(old->inode->flags & FS_DIR)) ||
                           (!(existing_tnode->inode->flags & FS_DIR) && (old->inode->flags & FS_DIR)))) {
        return -ENOTDIR;
    }

    if (existing_tnode && (existing_tnode->inode->flags & FS_DIR) && !dir_empty(existing_tnode->inode)) {
        return -ENOTEMPTY;
    }

    if (!old->inode->super_block->op || !old->inode->super_block->op->rename) {
        return -EINVAL;
    }

    // Destroy the existing tnode if necessary
    if (existing_tnode) {
        if (existing_tnode->inode == old->inode) {
            return 0;
        }

        if (((existing_tnode->inode->flags & FS_DIR) && !existing_tnode->inode->i_op->rmdir) || !existing_tnode->inode->i_op->unlink) {
            return -EINVAL;
        }

        spin_lock(&existing_tnode->inode->lock);

        int ret = 0;
        if (existing_tnode->inode->flags & FS_DIR) {
            ret = existing_tnode->inode->i_op->rmdir(existing_tnode);
        } else {
            ret = existing_tnode->inode->i_op->unlink(existing_tnode);
        }

        if (ret != 0) {
            return ret;
        }

        existing_tnode->inode->parent->inode->tnode_list = remove_tnode(existing_tnode->inode->parent->inode->tnode_list, existing_tnode);
        struct inode *inode = existing_tnode->inode;
        free(existing_tnode);

        drop_inode_reference(inode);
    }

    int ret = old->inode->super_block->op->rename(old, new_parent, new_path_last_slash + 1);
    if (ret != 0) {
        return ret;
    }

    struct tnode *old_parent = old->inode->parent;
    assert(old_parent);

    free(old->name);
    old->name = strdup(new_path_last_slash + 1);
    old->inode->parent = new_parent;
    new_parent->inode->tnode_list = add_tnode(new_parent->inode->tnode_list, old);
    old_parent->inode->tnode_list = remove_tnode(old_parent->inode->tnode_list, old);
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

struct file_descriptor fs_clone(struct file_descriptor desc) {
    if (desc.file == NULL) {
        return (struct file_descriptor) { NULL, 0 };
    }

    struct file *new_file = malloc(sizeof(struct file));
    memcpy(new_file, desc.file, sizeof(struct file));

    if (new_file->f_op->clone) {
        new_file->f_op->clone(new_file);
    }

    struct inode *inode = fs_inode_get(desc.file->device, desc.file->inode_idenifier);
    assert(inode);

    bump_inode_reference(inode);

    // NOTE: the new file should't have the same fdflags
    return (struct file_descriptor) { new_file, 0 };
}

int fs_access(const char *path, int mode) {
    assert(path);

    struct tnode *tnode = iname(path);
    if (!tnode) {
        return -ENOENT;
    }

    if (mode == F_OK) {
        return 0;
    }

    struct inode *inode = tnode->inode;
    if (mode &= R_OK && !(inode->mode & S_IRUSR)) {
        return EPERM;
    }

    if (mode &= W_OK && !(inode->mode & S_IWUSR)) {
        return EPERM;
    }

    if (mode &= X_OK && !(inode->mode & S_IXUSR)) {
        return EPERM;
    }

    return 0;
}

int fs_fcntl(struct file_descriptor *desc, int command, int arg) {
    assert(desc->file);

    switch (command) {
        case F_DUPFD_CLOEXEC:
            desc->fd_flags |= FD_CLOEXEC;
            // Fall through
        case F_DUPFD: {
            struct task *current = get_current_task();
            if (arg < 0 || arg > FOPEN_MAX) {
                return -EINVAL;
            }

            for (int i = arg; i < FOPEN_MAX; i++) {
                if (current->process->files[i].file == NULL) {
                    current->process->files[i] = fs_dup(*desc);
                    return i;
                }
            }

            return -EMFILE;
        }
        case F_GETFD:
            return desc->fd_flags;
        case F_SETFD:
            if (arg == 0 || arg == FD_CLOEXEC) {
                desc->fd_flags = arg;
                return 0;
            }

            return -EINVAL;
        case F_GETFL:
            return desc->file->open_flags;
        case F_SETFL:
            debug_log("fcntl: f_setfl: [ %#.8X ]\n", (unsigned int) arg);
            // FIXME: this should do validity checks and update the files capabilites
            desc->file->open_flags = arg;
            return 0;
        default:
            return -EINVAL;
    }
}

int fs_fstat(struct file *file, struct stat *stat_struct) {
    struct inode *inode = fs_inode_get(file->device, file->inode_idenifier);
    assert(inode);

    if (inode->i_op->stat) {
        return inode->i_op->stat(inode, stat_struct);
    }

    return -EPERM;
}

int fs_fchmod(struct file *file, mode_t mode) {
    struct inode *inode = fs_inode_get(file->device, file->inode_idenifier);
    assert(inode);

    if (inode->i_op->chmod) {
        return inode->i_op->chmod(inode, mode);
    }

    return -EPERM;
}

// NOTE: we don't have to write out to disk, because we only loose info
//       stored on the inode after rebooting, and at that point, the binding
//       task will no longer exist.
int fs_bind_socket_to_inode(struct inode *inode, unsigned long socket_id) {
    assert(inode->flags & FS_SOCKET && S_ISSOCK(inode->mode));
    inode->socket_id = socket_id;

    return 0;
}

struct file_descriptor fs_dup(struct file_descriptor desc) {
    if (desc.file == NULL) {
        return (struct file_descriptor) { NULL, 0 };
    }

    spin_lock(&desc.file->lock);
    assert(desc.file->ref_count > 0);
    desc.file->ref_count++;
    spin_unlock(&desc.file->lock);

    // NOTE: the new descriptor reset the FD_CLOEXEC flag
    return (struct file_descriptor) { desc.file, 0 };
}

void init_vfs() {
    init_fs_inode_store();

    init_initrd();
    init_dev();
    init_ext2();
    init_pipe();
    init_tmpfs();

    /* Mount INITRD as root */
    int error = fs_mount("", "/", "initrd");
    assert(error == 0);

    /* Mount dev at /dev */
    error = fs_mount("", "/dev", "dev");
    assert(error == 0);

    // Mount tmpfs at /tmp
    error = fs_mount("", "/tmp", "tmpfs");
    assert(error == 0);

    // Mount tmpfs at /dev/shm
    error = fs_mount("", "/dev/shm", "tmpfs");
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
    char **name_buffer = malloc(size * sizeof(char **));

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
    for (i--; i >= 0; i--) {
        strcat(ret, "/");
        strcat(ret, name_buffer[i]);
    }

    free(name_buffer);
    return ret;
}

bool fs_is_readable(struct file *file) {
    if (file->flags & FS_SOCKET) {
        struct socket_file_data *file_data = file->private_data;
        struct socket *socket = net_get_socket_by_id(file_data->socket_id);
        assert(socket);

        return socket->readable;
    }

    struct inode *inode = fs_inode_get(file->device, file->inode_idenifier);
    assert(inode);

    return inode->readable;
}

bool fs_is_writable(struct file *file) {
    if (file->flags & FS_SOCKET) {
        struct socket_file_data *file_data = file->private_data;
        struct socket *socket = net_get_socket_by_id(file_data->socket_id);
        assert(socket);

        return socket->writable;
    }

    struct inode *inode = fs_inode_get(file->device, file->inode_idenifier);
    assert(inode);

    return inode->writeable;
}

bool fs_is_exceptional(struct file *file) {
    if (file->flags & FS_SOCKET) {
        struct socket_file_data *file_data = file->private_data;
        struct socket *socket = net_get_socket_by_id(file_data->socket_id);
        assert(socket);

        return socket->exceptional;
    }

    struct inode *inode = fs_inode_get(file->device, file->inode_idenifier);
    assert(inode);

    return inode->excetional_activity;
}