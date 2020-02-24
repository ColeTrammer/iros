#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <sys/uio.h>

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
#include <kernel/util/validators.h>

// #define INODE_REF_COUNT_DEBUG

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
    if (--inode->ref_count <= 0) {
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

int fs_read_all_inode_with_buffer(struct inode *inode, void *buffer) {
    if (!inode->i_op->read_all) {
        return -EINVAL;
    }

    if (inode->i_op->lookup && inode->size == 0) {
        inode->i_op->lookup(inode, NULL);
    }

    if (inode->size == 0) {
        return 0;
    }

    return inode->i_op->read_all(inode, buffer);
}

int fs_read_all_inode(struct inode *inode, void **buffer, size_t *buffer_len) {
    if (!inode->i_op->read_all) {
        return -EINVAL;
    }

    if (inode->size == 0 && inode->i_op->lookup) {
        inode->i_op->lookup(inode, NULL);
    }

    if (inode->size == 0) {
        *buffer = NULL;
        if (buffer_len) {
            *buffer_len = 0;
        }

        return 0;
    }

    *buffer = malloc(inode->size);
    if (!*buffer) {
        return -ENOMEM;
    }

    int ret = inode->i_op->read_all(inode, *buffer);
    if (ret < 0) {
        return ret;
    }

    if (buffer_len) {
        *buffer_len = inode->size;
    }

    return 0;
}

int fs_read_all_path(const char *path, void **buffer, size_t *buffer_len, struct inode **inode) {
    struct tnode *tnode;
    int ret = iname(path, 0, &tnode);
    if (ret < 0) {
        return ret;
    }

    if (inode) {
        *inode = tnode->inode;
        if (!fs_inode_get(tnode->inode->device, tnode->inode->index)) {
            fs_inode_put(tnode->inode);
        }
    }

    assert(tnode->inode);
    return fs_read_all_inode(tnode->inode, buffer, buffer_len);
}

struct tnode *fs_root(void) {
    return root->super_block->root;
}

static int do_iname(const char *_path, int flags, struct tnode *t_root, struct tnode *t_parent, struct tnode **result, int *depth) {
    if (*depth >= 25) {
        return -ELOOP;
    }

    assert(t_root->inode != NULL);
    assert(t_root->inode->i_op != NULL);

    struct tnode *parent = t_root;
    char *path = flags & INAME_TAKE_OWNERSHIP_OF_PATH ? (char *) _path : strdup(_path);
    char *save_path = path;

    if (_path[0] != '/') {
        parent = t_parent;
        path--;
    }

    char *last_slash = strchr(path + 1, '/');

    /* Main VFS Loop */
    while (parent != NULL && path != NULL && path[1] != '\0') {
        if (parent->inode->flags & FS_LINK) {
            if (parent->inode->size == 0 && parent->inode->i_op->lookup) {
                parent->inode->i_op->lookup(parent->inode, NULL);
            }

            if (parent->inode->size == 0 || !parent->inode->i_op->read_all) {
                free(save_path);
                return -ENOENT;
            }

            char *link_path = malloc(parent->inode->size + 1);
            int ret = parent->inode->i_op->read_all(parent->inode, link_path);
            if (ret < 0) {
                free(link_path);
                free(save_path);
                return ret;
            }
            link_path[parent->inode->size] = '\0';

            if (strcmp(parent->name, link_path) == 0) {
                free(link_path);
                free(save_path);
                return -ELOOP;
            }

            (*depth)++;
            ret = do_iname(link_path, flags | INAME_TAKE_OWNERSHIP_OF_PATH, t_root, parent->inode->parent, &parent, depth);
            if (ret < 0) {
                free(save_path);
                return ret;
            }

            continue;
        }

        /* Exit if we're trying to lookup past a file */
        if (!(parent->inode->flags & FS_DIR)) {
            free(save_path);
            return -ENOTDIR;
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
        return -ENOENT;
    }

    struct inode *inode = parent->inode;

    /* Shouldn't let you at a / at the end of a file name or root (but only if the path is // and not /./) */
    if ((path != NULL && path >= save_path && path[0] == '/') &&
        ((inode->flags & FS_FILE) || (inode == inode->parent->inode && strlen(_path) == 2))) {
        free(save_path);
        return -ENOENT;
    }

    free(save_path);

    if ((flags & INAME_DONT_FOLLOW_TRAILING_SYMLINK) || !(inode->flags & FS_LINK)) {
        *result = parent;
        return 0;
    }

    if (inode->size == 0 && inode->i_op->lookup) {
        inode->i_op->lookup(inode, NULL);
    }

    if (inode->size == 0 || !inode->i_op->read_all) {
        return -ENOENT;
    }

    char *sym_path = malloc(inode->size + 1);

    int ret = inode->i_op->read_all(inode, sym_path);
    if (ret < 0) {
        free(sym_path);
        return ret;
    }
    sym_path[inode->size] = '\0';

    if (strcmp(parent->name, sym_path) == 0) {
        free(sym_path);
        return -ELOOP;
    }

    (*depth)++;
    ret = do_iname(sym_path, flags | INAME_TAKE_OWNERSHIP_OF_PATH, t_root, parent->inode->parent, result, depth);

    return ret;
}

int iname(const char *_path, int flags, struct tnode **result) {
    struct task *current = get_current_task();
    struct tnode *cwd = NULL;
    if (current && current->process && current->process->cwd) {
        cwd = current->process->cwd;
    }
    return iname_with_base(cwd, _path, flags, result);
}

int iname_with_base(struct tnode *base, const char *_path, int flags, struct tnode **result) {
    assert(root != NULL);
    assert(root->super_block != NULL);

    struct tnode *t_root = root->super_block->root;
    if (t_root == NULL) {
        return -ENOENT;
    }

    if (!base) {
        base = t_root;
    }

    int depth_storage = 0;
    return do_iname(_path, flags, t_root, base, result, &depth_storage);
}

struct tnode *fs_create(const char *file_name, mode_t mode, int *error) {
    mode &= ~get_current_task()->process->umask;

    char *path = malloc(strlen(file_name) + 1);
    strcpy(path, file_name);

    char *last_slash = strrchr(path, '/');
    struct tnode *tparent;

    int ret = 0;
    if (last_slash == path) {
        tparent = fs_root();
    } else if (last_slash == NULL) {
        tparent = get_current_task()->process->cwd;
        last_slash = path - 1;
    } else {
        *last_slash = '\0';
        ret = iname(path, 0, &tparent);
    }

    if (ret < 0) {
        free(path);
        *error = ret;
        return NULL;
    }

    struct mount *mount = tparent->inode->mounts;
    while (mount != NULL) {
        if (strcmp(mount->name, last_slash + 1) == 0) {
            free(path);
            *error = -EEXIST;
            return NULL;
        }

        mount = mount->next;
    }

    tparent->inode->i_op->lookup(tparent->inode, NULL);
    if (tparent->inode->tnode_list && find_tnode(tparent->inode->tnode_list, last_slash + 1) != NULL) {
        free(path);
        *error = -EEXIST;
        return NULL;
    }

    if (!tparent->inode->i_op->create) {
        free(path);
        *error = -EINVAL;
        return NULL;
    }

    debug_log("Adding to: [ %s, %p ]\n", tparent->name, tparent->inode);

    struct inode *inode = tparent->inode->i_op->create(tparent, last_slash + 1, mode, error);
    if (inode == NULL) {
        free(path);
        return NULL;
    }

    struct tnode *tnode = create_tnode(last_slash + 1, inode);
    tparent->inode->tnode_list = add_tnode(tparent->inode->tnode_list, tnode);

    free(path);
    return tnode;
}

struct file *fs_open(const char *file_name, int flags, mode_t mode, int *error) {
    return fs_openat(NULL, file_name, flags, mode, error);
}

struct file *fs_openat(struct tnode *base, const char *file_name, int flags, mode_t mode, int *error) {
    if (file_name == NULL) {
        *error = -EINVAL;
        return NULL;
    }

    struct tnode *tnode;
    int ret = iname_with_base(base, file_name, 0, &tnode);
    if (ret == -ENOENT) {
        if (flags & O_CREAT) {
            debug_log("Creating file: [ %s ]\n", file_name);

            ret = 0;
            tnode = fs_create(file_name, mode | S_IFREG, &ret);
            if (ret < 0) {
                *error = ret;
                return NULL;
            }
        } else {
            debug_log("File Not Found: [ %s ]\n", file_name);
            *error = ret;
            return NULL;
        }
    } else if (ret < 0) {
        *error = ret;
        return NULL;
    } else if (flags & O_EXCL) {
        *error = -EEXIST;
        return NULL;
    }

    if (!(tnode->inode->flags & FS_DIR) && (flags & O_DIRECTORY)) {
        *error = -ENOTDIR;
        return NULL;
    }

    if (tnode->inode->flags & FS_DIR && !(flags & O_DIRECTORY)) {
        *error = -EISDIR;
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
    file->open_flags = flags;
    file->abilities &= FS_FILE_CANT_SEEK;
    if (flags & O_RDWR) {
        file->abilities |= FS_FILE_CAN_WRITE | FS_FILE_CAN_READ;
    } else if (flags & O_RDONLY) {
        file->abilities |= FS_FILE_CAN_READ;
    } else if (flags & O_WRONLY) {
        file->abilities |= FS_FILE_CAN_WRITE;
    }

    /* Handle append mode */
    if (flags & O_APPEND) {
        fs_seek(file, 0, SEEK_END);
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
        if (file->abilities & FS_FILE_CANT_SEEK) {
            return file->f_op->read(file, 0, buffer, len);
        }

        ssize_t ret = file->f_op->read(file, file->position, buffer, len);
        if (ret > 0) {
            file->position += ret;
        }

        return ret;
    }

    if (file->flags & FS_DIR) {
        return default_dir_read(file, buffer, len);
    }

    return -EINVAL;
}

ssize_t fs_pread(struct file *file, void *buffer, size_t len, off_t offset) {
    if (file->abilities & FS_FILE_CANT_SEEK) {
        return -ESPIPE;
    }

    if (len == 0) {
        return 0;
    }

    assert(file);
    assert(file->f_op);
    if (file->f_op->read) {
        return file->f_op->read(file, offset, buffer, len);
    }

    return -EINVAL;
}

ssize_t fs_write(struct file *file, const void *buffer, size_t len) {
    if (len == 0) {
        return 0;
    }

    if (file->f_op->write) {
        if (file->abilities & FS_FILE_CANT_SEEK) {
            return file->f_op->write(file, 0, buffer, len);
        }

        ssize_t ret = file->f_op->write(file, file->position, buffer, len);
        if (ret > 0) {
            file->position += ret;
        }

        return ret;
    }

    return -EINVAL;
}

ssize_t fs_pwrite(struct file *file, const void *buffer, size_t len, off_t offset) {
    if (file->abilities & FS_FILE_CANT_SEEK) {
        return -ESPIPE;
    }

    if (len == 0) {
        return 0;
    }

    if (file->f_op->write) {
        return file->f_op->write(file, offset, buffer, len);
    }

    return -EINVAL;
}

ssize_t fs_readv(struct file *file, const struct iovec *vec, int item_count) {
    ssize_t ret = 0;

    // FIXME: this function should lock the file or inode somehow
    for (int i = 0; i < item_count; i++) {
        VALIDATE(vec[i].iov_base, vec[i].iov_len, validate_write);
        ssize_t res = fs_read(file, vec[i].iov_base, vec[i].iov_len);
        if (res < 0) {
            return res;
        }

        ret += res;
        if (res < (ssize_t) vec[i].iov_len) {
            break;
        }
    }

    return ret;
}

ssize_t fs_writev(struct file *file, const struct iovec *vec, int item_count) {
    ssize_t ret = 0;

    // FIXME: this function should lock the file or inode somehow
    for (int i = 0; i < item_count; i++) {
        VALIDATE(vec[i].iov_base, vec[i].iov_len, validate_read);
        ssize_t res = fs_write(file, vec[i].iov_base, vec[i].iov_len);
        if (res < 0) {
            return res;
        }

        ret += res;
        if (res < (ssize_t) vec[i].iov_len) {
            break;
        }
    }

    return ret;
}

off_t fs_seek(struct file *file, off_t offset, int whence) {
    if (file->abilities & FS_FILE_CANT_SEEK) {
        return -ESPIPE;
    }

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

static int do_stat(struct inode *inode, struct stat *stat_struct) {
    stat_struct->st_rdev = 0;
    stat_struct->st_nlink = 1;

    if (inode->i_op->stat) {
        int ret = inode->i_op->stat(inode, stat_struct);
        if (ret < 0) {
            return ret;
        }
    }

    stat_struct->st_dev = inode->device;
    stat_struct->st_ino = inode->index;
    stat_struct->st_mode = inode->mode;
    stat_struct->st_uid = inode->uid;
    stat_struct->st_gid = inode->gid;
    stat_struct->st_size = inode->size;
    stat_struct->st_atim = inode->access_time;
    stat_struct->st_ctim = inode->modify_time;
    stat_struct->st_mtim = inode->change_time;
    stat_struct->st_blksize = inode->super_block->block_size;
    stat_struct->st_blocks = (inode->size + inode->super_block->block_size - 1) / inode->super_block->block_size;

    return 0;
}

int fs_stat(const char *path, struct stat *stat_struct) {
    struct tnode *tnode;
    int ret = iname(path, 0, &tnode);
    if (ret < 0) {
        return -ENOENT;
    }

    return do_stat(tnode->inode, stat_struct);
}

int fs_lstat(const char *path, struct stat *stat_struct) {
    struct tnode *tnode;

    int ret = iname(path, INAME_DONT_FOLLOW_TRAILING_SYMLINK, &tnode);
    if (ret < 0) {
        return ret;
    }

    return do_stat(tnode->inode, stat_struct);
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
    struct tnode *tparent;

    int ret = 0;
    if (last_slash == path) {
        tparent = fs_root();
    } else if (last_slash == NULL) {
        tparent = get_current_task()->process->cwd;
        last_slash = path - 1;
    } else {
        *last_slash = '\0';
        ret = iname(path, 0, &tparent);
    }

    if (ret < 0) {
        free(path);
        return ret;
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

    struct tnode *tnode = create_tnode(last_slash + 1, inode);
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

    struct tnode *tnode;
    int ret = iname(path, INAME_DONT_FOLLOW_TRAILING_SYMLINK, &tnode);
    if (ret < 0) {
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
        debug_log("No i_op->unlink: [ %s ]\n", tnode->name);
        return -EINVAL;
    }

    spin_lock(&tnode->inode->lock);

    ret = tnode->inode->i_op->unlink(tnode);
    if (ret != 0) {
        return ret;
    }

    tnode->inode->parent->inode->tnode_list = remove_tnode(tnode->inode->parent->inode->tnode_list, tnode);
    struct inode *inode = tnode->inode;

    spin_unlock(&inode->lock);
    drop_tnode(tnode);
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

    struct tnode *tnode;
    int ret = iname(path, INAME_DONT_FOLLOW_TRAILING_SYMLINK, &tnode);
    if (ret < 0) {
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

    ret = tnode->inode->i_op->rmdir(tnode);
    if (ret != 0) {
        return ret;
    }

    struct inode *inode = tnode->inode;
    inode->parent->inode->tnode_list = remove_tnode(inode->parent->inode->tnode_list, tnode);
    drop_tnode(tnode);
    return 0;
}

int fs_chown(const char *path, uid_t uid, gid_t gid) {
    assert(path);

    if (get_current_task()->process->euid != 0) {
        return -EPERM;
    }

    struct tnode *tnode;
    int ret = iname(path, 0, &tnode);

    if (ret < 0) {
        return ret;
    }

    if (!tnode->inode->i_op->chown) {
        return -EPERM;
    }

    return tnode->inode->i_op->chown(tnode->inode, uid, gid);
}

int fs_chmod(const char *path, mode_t mode) {
    assert(path);

    struct tnode *tnode;
    int ret = iname(path, 0, &tnode);

    if (ret < 0) {
        return ret;
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

int fs_utimes(const char *path, const struct timeval *times) {
    struct tnode *tnode;
    int ret = iname(path, 0, &tnode);
    if (ret < 0) {
        return ret;
    }

    if (!tnode->inode->i_op->utimes) {
        return -EPERM;
    }

    return tnode->inode->i_op->utimes(tnode->inode, times);
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

int fs_rename(const char *old_path, const char *new_path) {
    assert(old_path);
    assert(new_path);

    debug_log("Rename: [ %s, %s ]\n", old_path, new_path);

    struct tnode *old;
    int ret = iname(old_path, 0, &old);
    if (ret < 0) {
        return ret;
    }

    char *new_path_last_slash = strrchr(new_path, '/');
    assert(new_path_last_slash);

    struct tnode *new_parent;
    if (new_path_last_slash == new_path) {
        new_parent = root->super_block->root;
    } else {
        *new_path_last_slash = '\0';
        iname(new_path, 0, &new_parent);
        *new_path_last_slash = '/';
    }

    if (new_parent == NULL) {
        return -ENOENT;
    }

    if (old->inode->super_block != new_parent->inode->super_block) {
        return -EXDEV;
    }

    struct tnode *existing_tnode = NULL;
    iname(new_path, 0, &existing_tnode);
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
        drop_tnode(existing_tnode);
    }

    ret = old->inode->super_block->op->rename(old, new_parent, new_path_last_slash + 1);
    if (ret != 0) {
        return ret;
    }

    struct tnode *old_parent = old->inode->parent;
    assert(old_parent);

    spin_lock(&old->lock);
    free(old->name);
    old->name = strdup(new_path_last_slash + 1);
    old->inode->parent = new_parent;
    new_parent->inode->tnode_list = add_tnode(new_parent->inode->tnode_list, old);
    old_parent->inode->tnode_list = remove_tnode(old_parent->inode->tnode_list, old);
    spin_lock(&old->lock);
    return 0;
}

static int do_statvfs(struct super_block *sb, struct statvfs *buf) {
    buf->f_bsize = sb->block_size;
    buf->f_frsize = sb->block_size;

    buf->f_blocks = sb->num_blocks;
    buf->f_bfree = sb->free_blocks;
    buf->f_bavail = sb->available_blocks;

    buf->f_files = sb->num_inodes;
    buf->f_ffree = sb->free_inodes;
    buf->f_favail = sb->available_blocks;

    buf->f_fsid = sb->device;
    buf->f_flag = sb->flags;
    buf->f_namemax = NAME_MAX;

    return 0;
}

int fs_fstatvfs(struct file *file, struct statvfs *buf) {
    struct inode *inode = fs_inode_get(file->device, file->inode_idenifier);
    return do_statvfs(inode->super_block, buf);
}

int fs_statvfs(const char *path, struct statvfs *buf) {
    struct tnode *tnode;
    int ret = iname(path, 0, &tnode);
    if (ret < 0) {
        return ret;
    }

    return do_statvfs(tnode->inode->super_block, buf);
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
                int ret = iname(path_copy, 0, &mount_on);
                if (ret < 0) {
                    free(path_copy);
                    return ret;
                }
            }

            if (mount_on == NULL || !(mount_on->inode->flags & FS_DIR)) {
                free(path_copy);
                return -ENOTDIR;
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

    struct tnode *tnode;
    int ret = iname(path, 0, &tnode);
    if (ret < 0) {
        return ret;
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

ssize_t fs_readlink(const char *path, char *buf, size_t bufsiz) {
    struct tnode *link;
    {
        int ret = iname(path, INAME_DONT_FOLLOW_TRAILING_SYMLINK, &link);
        if (ret < 0) {
            return ret;
        }
    }

    if (!(link->inode->flags & FS_LINK)) {
        return -EINVAL;
    }

    void *buffer;
    size_t buffer_len;
    int ret = fs_read_all_inode(link->inode, &buffer, &buffer_len);
    if (ret < 0 || buffer == NULL) {
        return ret;
    }

    size_t to_write = MIN(bufsiz, buffer_len);
    memcpy(buf, buffer, to_write);

    free(buffer);
    return to_write;
}

int fs_fstat(struct file *file, struct stat *stat_struct) {
    struct inode *inode = fs_inode_get(file->device, file->inode_idenifier);
    assert(inode);

    return do_stat(inode, stat_struct);
}

int fs_fchmod(struct file *file, mode_t mode) {
    struct inode *inode = fs_inode_get(file->device, file->inode_idenifier);
    assert(inode);

    if (inode->i_op->chmod) {
        return inode->i_op->chmod(inode, mode);
    }

    return -EPERM;
}

int fs_symlink(const char *target, const char *linkpath) {
    char *path = malloc(strlen(linkpath) + 1);
    strcpy(path, linkpath);

    char *last_slash = strrchr(path, '/');
    struct tnode *tparent;

    int ret = 0;
    if (last_slash == path) {
        tparent = fs_root();
    } else if (last_slash == NULL) {
        tparent = get_current_task()->process->cwd;
        last_slash = path - 1;
    } else {
        *last_slash = '\0';
        ret = iname(path, 0, &tparent);
    }

    if (ret < 0) {
        free(path);
        return ret;
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

    if (!tparent->inode->i_op->symlink) {
        free(path);
        return -EINVAL;
    }

    debug_log("Adding symlink to: [ %s ]\n", tparent->name);

    int error = 0;
    struct inode *inode = tparent->inode->i_op->symlink(tparent, last_slash + 1, target, &error);
    if (inode == NULL) {
        free(path);
        return error;
    }

    struct tnode *tnode = create_tnode(last_slash + 1, inode);
    tparent->inode->tnode_list = add_tnode(tparent->inode->tnode_list, tnode);

    free(path);
    return 0;
}

int fs_link(const char *oldpath, const char *newpath) {
    struct tnode *target;

    int ret = iname(oldpath, 0, &target);
    if (ret < 0) {
        return ret;
    }

    char *path = malloc(strlen(newpath) + 1);
    strcpy(path, newpath);

    char *last_slash = strrchr(path, '/');
    struct tnode *tparent;

    if (last_slash == path) {
        tparent = fs_root();
    } else if (last_slash == NULL) {
        tparent = get_current_task()->process->cwd;
        last_slash = path - 1;
    } else {
        *last_slash = '\0';
        ret = iname(path, 0, &tparent);
    }

    if (ret < 0) {
        free(path);
        return ret;
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

    if (!tparent->inode->i_op->link) {
        free(path);
        return -EINVAL;
    }

    debug_log("Adding hard link to: [ %s ]\n", tparent->name);

    ret = tparent->inode->i_op->link(tparent, last_slash + 1, target);
    if (ret < 0) {
        return ret;
    }

    bump_inode_reference(target->inode);

    struct tnode *tnode = create_tnode(last_slash + 1, target->inode);
    tparent->inode->tnode_list = add_tnode(tparent->inode->tnode_list, tnode);

    free(path);
    return 0;
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

char *get_tnode_path(struct tnode *tnode) {
    /* Check if root */
    if (tnode->inode == tnode->inode->parent->inode) {
        char *ret = malloc(2);
        strcpy(ret, "/");
        return ret;
    }

    ssize_t size = 15;
    char **name_buffer = malloc(size * sizeof(char **));

    ssize_t len = 1;
    ssize_t i;
    for (i = 0; tnode->inode->parent->inode != tnode->inode; i++) {
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

struct tnode *fs_get_tnode_for_file(struct file *file) {
    assert(file);
    struct inode *inode = fs_inode_get(file->device, file->inode_idenifier);
    if (!inode) {
        return NULL;
    }

    struct inode *parent = inode->parent->inode;
    if (parent == inode) {
        return fs_root();
    }

    struct tnode_list *child = parent->tnode_list;
    while (child) {
        if (child->tnode->inode == inode) {
            return child->tnode;
        }
        child = child->next;
    }

    return NULL;
}