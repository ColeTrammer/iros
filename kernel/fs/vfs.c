#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <sys/uio.h>

#include <kernel/fs/cached_dirent.h>
#include <kernel/fs/dev.h>
#include <kernel/fs/ext2.h>
#include <kernel/fs/file_system.h>
#include <kernel/fs/initrd.h>
#include <kernel/fs/inode.h>
#include <kernel/fs/pipe.h>
#include <kernel/fs/procfs.h>
#include <kernel/fs/tmp.h>
#include <kernel/fs/vfs.h>
#include <kernel/hal/output.h>
#include <kernel/mem/inode_vm_object.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/net/socket.h>
#include <kernel/proc/task.h>
#include <kernel/util/validators.h>

// #define INAME_DEBUG
// #define INODE_REF_COUNT_DEBUG

static struct file_system *file_systems;
static struct mount *root;

struct inode *bump_inode_reference(struct inode *inode) {
    int fetched_ref_count = atomic_fetch_add(&inode->ref_count, 1);
    (void) fetched_ref_count;

#ifdef INODE_REF_COUNT_DEBUG
    debug_log("+Ref count: [ %lu, %llu, %d ]\n", inode->device, inode->index, fetched_ref_count + 1);
#endif /* INODE_REF_COUNT_DEBUG */

    assert(fetched_ref_count > 0);
    return inode;
}

void drop_inode_reference(struct inode *inode) {
    int fetched_ref_count = atomic_fetch_sub(&inode->ref_count, 1);

#ifdef INODE_REF_COUNT_DEBUG
    debug_log("-Ref count: [ %lu, %llu, %d ]\n", inode->device, inode->index, fetched_ref_count - 1);
#endif /* INODE_REF_COUNT_DEBUG */

    // Only delete inode if it's refcount is zero
    assert(fetched_ref_count > 0);
    if (fetched_ref_count == 1) {
#ifdef INODE_REF_COUNT_DEBUG
        debug_log("Destroying inode: [ %lu, %llu ]\n", inode->device, inode->index);
#endif /* INODE_REF_COUNT_DEBUG */
        if (inode->i_op->on_inode_destruction) {
            inode->i_op->on_inode_destruction(inode);
        }

        if (inode->dirent_cache) {
            fs_destroy_dirent_cache(inode->dirent_cache);
        }

        free(inode);
        return;
    }
}

int fs_read_all_inode_with_buffer(struct inode *inode, void *buffer) {
    if (!inode->i_op->read_all) {
        return -EINVAL;
    }

    if (inode->i_op->lookup) {
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

    if (inode->i_op->lookup) {
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

int fs_read_all_path(const char *path, void **buffer, size_t *buffer_len, struct tnode **tnode_out) {
    struct tnode *tnode = NULL;
    int ret = iname(path, 0, &tnode);
    if (ret < 0) {
        return ret;
    }

    struct inode *inode = tnode->inode;
    if (tnode_out) {
        *tnode_out = tnode;
    } else {
        drop_tnode(tnode);
    }

    assert(inode);
    debug_log("reading from: [ %s ]\n", path);
    return fs_read_all_inode(inode, buffer, buffer_len);
}

static struct tnode *t_root;

struct tnode *fs_root(void) {
    return t_root;
}

static int do_iname(const char *_path, int flags, struct tnode *t_root, struct tnode *t_parent, struct tnode **result, int *depth) {
    if (*depth >= 25) {
        return -ELOOP;
    }

#ifdef INAME_DEBUG
    debug_log("path: [ %s ]\n", _path);
#endif /* INAME_DEBUG */

    assert(t_root->inode != NULL);
    assert(t_root->inode->i_op != NULL);

    char *path = flags & INAME_TAKE_OWNERSHIP_OF_PATH ? (char *) _path : strdup(_path);
    char *save_path = path;

    struct tnode *parent;
    if (_path[0] != '/') {
        parent = bump_tnode(t_parent);
        path--;
    } else {
        parent = bump_tnode(t_root);
    }

    char *last_slash = strchr(path + 1, '/');

    /* Main VFS Loop */
    while (parent != NULL && path != NULL && path[1] != '\0') {
        if (parent->inode->flags & FS_LINK) {
            if (parent->inode->i_op->lookup) {
                parent->inode->i_op->lookup(parent->inode, NULL);
            }

            if (parent->inode->size == 0 || !parent->inode->i_op->read_all) {
                free(save_path);
                drop_tnode(parent);
                return -ENOENT;
            }

            char *link_path = malloc(parent->inode->size + 1);
            int ret = parent->inode->i_op->read_all(parent->inode, link_path);
            if (ret < 0) {
                free(link_path);
                free(save_path);
                drop_tnode(parent);
                return ret;
            }
            link_path[parent->inode->size] = '\0';

            if (strcmp(parent->name, link_path) == 0) {
                free(link_path);
                free(save_path);
                drop_tnode(parent);
                return -ELOOP;
            }

            (*depth)++;
            struct tnode *parent_to_pass = bump_tnode(parent->parent);
            drop_tnode(parent);
            ret = do_iname(link_path, flags | INAME_TAKE_OWNERSHIP_OF_PATH, t_root, parent_to_pass, &parent, depth);
            drop_tnode(parent_to_pass);
            if (ret < 0) {
                free(save_path);
                return ret;
            }

            continue;
        }

        /* Exit if we're trying to lookup past a file */
        if (!(parent->inode->flags & FS_DIR)) {
            drop_tnode(parent);
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
            struct tnode *next = bump_tnode(parent->parent);
            drop_tnode(parent);
            parent = next;
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

            if (strcmp(mount->name, path + 1) == 0) {
                parent = create_tnode(mount->name, parent, mount->super_block->root);
                goto vfs_loop_end;
            }

            mount = mount->next;
        }

        /* Check using lookup */
        assert(parent->inode);
        assert(parent->inode->i_op);
        assert(parent->inode->i_op->lookup);

#ifdef INAME_DEBUG
        debug_log("looking up: [ %s, %s ]\n", parent->name, path + 1);
#endif /* INAME_DEBUG */
        struct inode *inode = parent->inode->i_op->lookup(parent->inode, path + 1);
        if (!inode) {
            drop_tnode(parent);
            parent = NULL;
            break;
        }
        parent = create_tnode(path + 1, parent, inode);

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
        ((inode->flags & FS_FILE) || (parent->parent == NULL && strlen(_path) == 2))) {
        free(save_path);
        drop_tnode(parent);
        return -ENOENT;
    }

    free(save_path);

    if ((flags & INAME_DONT_FOLLOW_TRAILING_SYMLINK) || !(inode->flags & FS_LINK)) {
        *result = parent;
        return 0;
    }

    if (inode->i_op->lookup) {
        inode->i_op->lookup(inode, NULL);
    }

    if (inode->size == 0 || !inode->i_op->read_all) {
        drop_tnode(parent);
        return -ENOENT;
    }

    char *sym_path = malloc(inode->size + 1);

    int ret = inode->i_op->read_all(inode, sym_path);
    if (ret < 0) {
        free(sym_path);
        drop_tnode(parent);
        return ret;
    }
    sym_path[inode->size] = '\0';

    if (strcmp(parent->name, sym_path) == 0) {
        free(sym_path);
        drop_tnode(parent);
        return -ELOOP;
    }

    (*depth)++;
    struct tnode *parent_to_pass = bump_tnode(parent->parent);
    drop_tnode(parent);
    ret = do_iname(sym_path, flags | INAME_TAKE_OWNERSHIP_OF_PATH, t_root, parent_to_pass, result, depth);
    drop_tnode(parent_to_pass);

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

    struct tnode *t_root = fs_root();
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
        tparent = bump_tnode(fs_root());
    } else if (last_slash == NULL) {
        tparent = bump_tnode(get_current_task()->process->cwd);
        last_slash = path - 1;
    } else {
        *last_slash = '\0';
        ret = iname(path, 0, &tparent);
        if (ret < 0) {
            free(path);
            *error = ret;
            return NULL;
        }
    }

    struct mount *mount = tparent->inode->mounts;
    while (mount != NULL) {
        if (strcmp(mount->name, last_slash + 1) == 0) {
            free(path);
            drop_tnode(tparent);
            *error = -EEXIST;
            return NULL;
        }

        mount = mount->next;
    }

    tparent->inode->i_op->lookup(tparent->inode, NULL);
    if (fs_lookup_in_cache(tparent->inode->dirent_cache, last_slash + 1) != NULL) {
        drop_tnode(tparent);
        free(path);
        *error = -EEXIST;
        return NULL;
    }

    if (!tparent->inode->i_op->create) {
        drop_tnode(tparent);
        free(path);
        *error = -EINVAL;
        return NULL;
    }

    debug_log("Adding to: [ %s, %p ]\n", tparent->name, tparent->inode);

    struct inode *inode = tparent->inode->i_op->create(tparent, last_slash + 1, mode, error);
    if (inode == NULL) {
        drop_tnode(tparent);
        free(path);
        return NULL;
    }

    fs_put_dirent_cache(tparent->inode->dirent_cache, inode, last_slash + 1, strlen(last_slash + 1));

    struct tnode *tnode = create_tnode(last_slash + 1, tparent, inode);
    free(path);
    return tnode;
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
        drop_tnode(tnode);
        *error = -EEXIST;
        return NULL;
    }

    if (!(tnode->inode->flags & FS_DIR) && (flags & O_DIRECTORY)) {
        drop_tnode(tnode);
        *error = -ENOTDIR;
        return NULL;
    }

    if (tnode->inode->flags & FS_DIR && !(flags & O_DIRECTORY)) {
        drop_tnode(tnode);
        *error = -EISDIR;
        return NULL;
    }

    struct file *file = tnode->inode->i_op->open(tnode->inode, flags, error);
    if (file == NULL) {
        drop_tnode(tnode);
        return file;
    }

    // FIXME: this is a hack to allow calls to fs_open to return a call to fs_open.
    //        functions in the device file system that fake being a symlink (like
    //        ptmx) rely on this functionality, and we cannot overwrite the file's
    //        information here or the file will just be incorrect and memory will
    //        be leaked.
    if (file->tnode) {
        drop_tnode(tnode);
        return file;
    }

    file->inode = bump_inode_reference(tnode->inode);
    init_spinlock(&file->lock);
    file->ref_count = 1;
    file->open_flags = flags;
    file->abilities &= FS_FILE_CANT_SEEK;
    file->tnode = tnode;
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

    struct inode *inode = fs_file_inode(file);

    int fetched_ref_count = atomic_fetch_sub(&file->ref_count, 1);
    assert(fetched_ref_count > 0);
    if (fetched_ref_count == 1) {
        if (inode) {
            drop_inode_reference(inode);
        }

        if (file->tnode) {
            drop_tnode(file->tnode);
        }

        int error = 0;
        if (file->f_op->close) {
            error = file->f_op->close(file);
        }

        free(file);
        return error;
    }

    return 0;
}

/* Default dir read: works for file systems completely cached in memory */
static ssize_t default_dir_read(struct file *file, void *buffer, size_t len) {
    if (len != sizeof(struct dirent)) {
        return -EINVAL;
    }

    struct inode *inode = fs_file_inode(file);

    struct dirent *entry = (struct dirent *) buffer;
    if (file->position == 0) {
        entry->d_ino = inode->index;
        strcpy(entry->d_name, ".");
        file->position++;
        return len;
    } else if (file->position == 1) {
        entry->d_ino = file->tnode->parent ? file->tnode->parent->inode->index : inode->index;
        strcpy(entry->d_name, "..");
        file->position++;
        return len;
    }

    assert(inode->i_op->lookup);
    inode->i_op->lookup(inode, NULL);

    spin_lock(&inode->lock);
    struct cached_dirent *tnode = fs_lookup_in_cache_with_index(inode->dirent_cache, file->position - 2);

    if (!tnode) {
        /* Traverse mount points as well */
        size_t num = fs_get_dirent_cache_size(inode->dirent_cache) + 2;
        size_t mount_index = file->position - num;
        size_t i = 0;
        struct mount *mount = inode->mounts;
        while (mount != NULL) {
            if (i++ == mount_index) {
                file->position++;

                entry->d_ino = mount->super_block->root->index;
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
        struct inode *inode = fs_file_inode(file);
        new_position = inode->size + offset;
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

size_t fs_file_size(struct file *file) {
    struct inode *inode = fs_file_inode(file);
    if (!inode) {
        return 0;
    }

    if (inode->i_op->lookup) {
        inode->i_op->lookup(inode, NULL);
    }
    return inode->size;
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

    if (inode->i_op->lookup) {
        inode->i_op->lookup(inode, NULL);
    }

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

int fs_fstatat(struct tnode *base, const char *file_name, struct stat *stat_struct, int flags) {
    if (flags & AT_EMPTY_PATH) {
        return do_stat(base->inode, stat_struct);
    }

    if (!file_name) {
        return -EFAULT;
    }

    if (!(base->inode->flags & FS_DIR)) {
        return -ENOTDIR;
    }

    struct tnode *tnode;
    int ret = iname_with_base(base, file_name, (flags & AT_SYMLINK_NOFOLLOW) ? INAME_DONT_FOLLOW_TRAILING_SYMLINK : 0, &tnode);
    if (ret < 0) {
        return ret;
    }

    struct inode *inode = tnode->inode;
    drop_tnode(tnode);
    return do_stat(inode, stat_struct);
}

int fs_ioctl(struct file *file, unsigned long request, void *argp) {
    struct inode *inode = fs_file_inode(file);
    if (!inode) {
        return -ENOTTY;
    }

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

    size_t length = strlen(_path);
    char *path = malloc(length + 1);
    memcpy(path, _path, length);
    path[length] = '\0';

    char *last_slash;
    do {
        last_slash = strrchr(path, '/');
    } while (last_slash && (last_slash == path + length - 1) && (*last_slash = '\0', length--, 1));
    struct tnode *tparent;

    int ret = 0;
    if (last_slash == path) {
        tparent = bump_tnode(fs_root());
    } else if (last_slash == NULL) {
        tparent = bump_tnode(get_current_task()->process->cwd);
        last_slash = path - 1;
    } else {
        *last_slash = '\0';
        ret = iname(path, 0, &tparent);
        if (ret < 0) {
            free(path);
            return ret;
        }
    }

    if (tparent->inode->flags & FS_FILE) {
        drop_tnode(tparent);
        free(path);
        return -EINVAL;
    }

    struct mount *mount = tparent->inode->mounts;
    while (mount != NULL) {
        if (strcmp(mount->name, last_slash + 1) == 0) {
            free(path);
            drop_tnode(tparent);
            return -EEXIST;
        }

        mount = mount->next;
    }

    tparent->inode->i_op->lookup(tparent->inode, NULL);
    if (fs_lookup_in_cache(tparent->inode->dirent_cache, last_slash + 1) != NULL) {
        free(path);
        drop_tnode(tparent);
        return -EEXIST;
    }

    if (!tparent->inode->i_op->mkdir) {
        free(path);
        drop_tnode(tparent);
        return -EINVAL;
    }

    debug_log("Adding dir to: [ %s ]\n", tparent->name);

    int error = 0;
    struct inode *inode = tparent->inode->i_op->mkdir(tparent, last_slash + 1, mode | S_IFDIR, &error);
    if (inode == NULL) {
        drop_tnode(tparent);
        free(path);
        return error;
    }

    fs_put_dirent_cache(tparent->inode->dirent_cache, inode, last_slash + 1, strlen(last_slash + 1));

    drop_tnode(tparent);
    free(path);
    return 0;
}

int fs_create_pipe(struct file *pipe_files[2]) {
    struct inode *pipe_inode = pipe_new_inode();
    int error = 0;
    pipe_files[0] = pipe_inode->i_op->open(pipe_inode, O_RDONLY, &error);
    pipe_files[0]->abilities |= FS_FILE_CAN_READ;
    pipe_files[0]->inode = pipe_inode;
    init_spinlock(&pipe_files[0]->lock);
    pipe_files[0]->ref_count = 1;
    if (error != 0) {
        return error;
    }

    pipe_files[1] = pipe_inode->i_op->open(pipe_inode, O_WRONLY, &error);
    pipe_files[1]->abilities |= FS_FILE_CAN_WRITE;
    pipe_files[1]->inode = pipe_inode;
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
        return ret;
    }

    // Can't remove a unix socket that is being currently being used
    if (tnode->inode->socket_id != 0) {
        drop_tnode(tnode);
        return -EBUSY;
    }

    if (tnode->inode->flags & FS_DIR) {
        debug_log("Name: [ %s, %u ]\n", tnode->name, tnode->inode->flags);
        drop_tnode(tnode);
        return -EISDIR;
    }

    if (tnode->inode->i_op->unlink == NULL) {
        debug_log("No i_op->unlink: [ %s ]\n", tnode->name);
        drop_tnode(tnode);
        return -EINVAL;
    }

    spin_lock(&tnode->inode->lock);
    ret = tnode->inode->i_op->unlink(tnode);
    if (ret != 0) {
        drop_tnode(tnode);
        spin_unlock(&tnode->inode->lock);
        return ret;
    }
    spin_unlock(&tnode->inode->lock);

    fs_del_dirent_cache(tnode->parent->inode->dirent_cache, tnode->name);
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

    inode->i_op->lookup(inode, NULL);
    return fs_get_dirent_cache_size(inode->dirent_cache) == 0;
}

int fs_rmdir(const char *path) {
    assert(path);

    struct tnode *tnode;
    int ret = iname(path, INAME_DONT_FOLLOW_TRAILING_SYMLINK, &tnode);
    if (ret < 0) {
        return -ENOENT;
    }

    if (!(tnode->inode->flags & FS_DIR)) {
        drop_tnode(tnode);
        return -ENOTDIR;
    }

    if (!dir_empty(tnode->inode)) {
        drop_tnode(tnode);
        return -ENOTEMPTY;
    }

    if (tnode->inode->i_op->rmdir == NULL) {
        drop_tnode(tnode);
        return -EINVAL;
    }

    spin_lock(&tnode->inode->lock);

    ret = tnode->inode->i_op->rmdir(tnode);
    if (ret != 0) {
        drop_tnode(tnode);
        return ret;
    }

    fs_del_dirent_cache(tnode->parent->inode->dirent_cache, tnode->name);
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
        drop_tnode(tnode);
        return -EPERM;
    }

    struct inode *inode = tnode->inode;
    drop_tnode(tnode);
    return tnode->inode->i_op->chown(inode, uid, gid);
}

int fs_chmod(const char *path, mode_t mode) {
    assert(path);

    struct tnode *tnode;
    int ret = iname(path, 0, &tnode);

    if (ret < 0) {
        return ret;
    }

    if (!tnode->inode->i_op->chmod) {
        drop_tnode(tnode);
        return -EPERM;
    }

    // Don't yet support SETUID and SETGID
    mode &= 0777;

    // Retain type information
    mode |= tnode->inode->mode & ~07777;

    struct inode *inode = tnode->inode;
    drop_tnode(tnode);
    return tnode->inode->i_op->chmod(inode, mode);
}

int fs_utimes(const char *path, const struct timeval *times) {
    struct tnode *tnode;
    int ret = iname(path, 0, &tnode);
    if (ret < 0) {
        return ret;
    }

    if (!tnode->inode->i_op->utimes) {
        drop_tnode(tnode);
        return -EPERM;
    }

    struct inode *inode = tnode->inode;
    drop_tnode(tnode);
    return tnode->inode->i_op->utimes(inode, times);
}

intptr_t fs_default_mmap(void *addr, size_t len, int prot, int flags, struct inode *inode, off_t offset) {
    len += offset - (offset & ~0xFFF);
    offset &= ~0xFFF;
    len = ((len + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE;

    if (inode->i_op->lookup) {
        inode->i_op->lookup(inode, NULL);
    }

    struct vm_region *region = map_region(addr, len, prot, VM_DEVICE_MEMORY_MAP_DONT_FREE_PHYS_PAGES);
    if (!region) {
        return -ENOMEM;
    }

    assert(offset % PAGE_SIZE == 0);
    assert(len % PAGE_SIZE == 0);

    struct vm_object *object = NULL;
    if (flags & MAP_PRIVATE) {
        object = vm_create_inode_object(inode, flags);
    } else {
        region->flags |= VM_SHARED;
        if (!inode->vm_object) {
            object = vm_create_inode_object(inode, flags);
            inode->vm_object = object;
        } else {
            object = bump_vm_object(inode->vm_object);
        }
    }

    assert(object);
    region->vm_object = object;
    region->vm_object_offset = offset;

    int ret = vm_map_region_with_object(region);
    if (ret < 0) {
        return (intptr_t) ret;
    }

    return region->start;
}

intptr_t fs_mmap(void *addr, size_t len, int prot, int flags, struct file *file, off_t offset) {
    if (file == NULL) {
        return -EINVAL;
    }

    struct inode *inode = fs_file_inode(file);
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
        new_parent = fs_root();
    } else {
        *new_path_last_slash = '\0';
        iname(new_path, 0, &new_parent);
        *new_path_last_slash = '/';
    }

    if (new_parent == NULL) {
        drop_tnode(old);
        return -ENOENT;
    }

    if (old->inode->super_block != new_parent->inode->super_block) {
        drop_tnode(old);
        drop_tnode(new_parent);
        return -EXDEV;
    }

    struct tnode *existing_tnode = NULL;
    ret = iname(new_path, 0, &existing_tnode);
    if (ret < 0) {
        drop_tnode(old);
        drop_tnode(new_parent);
        return ret;
    }

    if ((((existing_tnode->inode->flags & FS_DIR) && !(old->inode->flags & FS_DIR)) ||
         (!(existing_tnode->inode->flags & FS_DIR) && (old->inode->flags & FS_DIR)))) {
        drop_tnode(old);
        drop_tnode(new_parent);
        drop_tnode(existing_tnode);
        return -ENOTDIR;
    }

    if (existing_tnode && (existing_tnode->inode->flags & FS_DIR) && !dir_empty(existing_tnode->inode)) {
        drop_tnode(old);
        drop_tnode(new_parent);
        drop_tnode(existing_tnode);
        return -ENOTEMPTY;
    }

    if (!old->inode->super_block->op || !old->inode->super_block->op->rename) {
        drop_tnode(old);
        drop_tnode(new_parent);
        drop_tnode(existing_tnode);
        return -EINVAL;
    }

    // Destroy the existing tnode if necessary
    if (existing_tnode) {
        if (existing_tnode->inode == old->inode) {
            drop_tnode(old);
            drop_tnode(new_parent);
            drop_tnode(existing_tnode);
            return 0;
        }

        if (((existing_tnode->inode->flags & FS_DIR) && !existing_tnode->inode->i_op->rmdir) || !existing_tnode->inode->i_op->unlink) {
            drop_tnode(old);
            drop_tnode(new_parent);
            drop_tnode(existing_tnode);
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
            drop_tnode(old);
            drop_tnode(new_parent);
            drop_tnode(existing_tnode);
            return ret;
        }

        struct inode *parent = existing_tnode->parent->inode;
        fs_del_dirent_cache(parent->dirent_cache, existing_tnode->name);
        drop_tnode(existing_tnode);
    }

    ret = old->inode->super_block->op->rename(old, new_parent, new_path_last_slash + 1);
    if (ret != 0) {
        drop_tnode(old);
        drop_tnode(new_parent);
        return ret;
    }

    struct tnode *old_parent = old->parent;
    assert(old_parent);

    fs_put_dirent_cache(new_parent->inode->dirent_cache, old->inode, new_path_last_slash + 1, strlen(new_path_last_slash + 1));
    fs_del_dirent_cache(old_parent->inode->dirent_cache, old->name);
    drop_tnode(old);
    drop_tnode(new_parent);
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
    struct inode *inode = fs_file_inode(file);
    return do_statvfs(inode->super_block, buf);
}

int fs_statvfs(const char *path, struct statvfs *buf) {
    struct tnode *tnode;
    int ret = iname(path, 0, &tnode);
    if (ret < 0) {
        return ret;
    }

    struct inode *inode = tnode->inode;
    drop_tnode(tnode);
    return do_statvfs(inode->super_block, buf);
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

                /* For now, when mounting as / when there is already something mounted,
                   we will just move things around instead of unmounting what was
                   already there */
                if (root != NULL) {
                    mount->super_block->root->mounts = root;
                    root->next = root->super_block->root->mounts;
                    root->super_block->root->mounts = NULL;

                    root->name = root->fs->name;
                }

                root = mount;

                if (t_root) {
                    drop_tnode(t_root);
                }
                t_root = create_root_tnode(mount->super_block->root);

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
                mount_on = bump_tnode(fs_root());
            } else {
                int ret = iname(path_copy, 0, &mount_on);
                if (ret < 0) {
                    free(path_copy);
                    return ret;
                }
            }

            if (mount_on == NULL || !(mount_on->inode->flags & FS_DIR)) {
                free(path_copy);
                drop_tnode(mount_on);
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

            free(path_copy);
            drop_tnode(mount_on);
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
    if (new_file->tnode) {
        bump_tnode(new_file->tnode);
    }

    if (new_file->f_op->clone) {
        new_file->f_op->clone(new_file);
    }

    struct inode *inode = fs_file_inode(desc.file);
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

    struct inode *inode = tnode->inode;
    drop_tnode(tnode);
    if (mode == F_OK) {
        return 0;
    }

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
        drop_tnode(link);
        return -EINVAL;
    }

    void *buffer;
    size_t buffer_len;
    int ret = fs_read_all_inode(link->inode, &buffer, &buffer_len);
    if (ret < 0 || buffer == NULL) {
        drop_tnode(link);
        return ret;
    }

    size_t to_write = MIN(bufsiz, buffer_len);
    memcpy(buf, buffer, to_write);

    free(buffer);
    drop_tnode(link);
    return to_write;
}

int fs_fchmod(struct file *file, mode_t mode) {
    struct inode *inode = fs_file_inode(file);
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
        tparent = bump_tnode(fs_root());
    } else if (last_slash == NULL) {
        tparent = bump_tnode(get_current_task()->process->cwd);
        last_slash = path - 1;
    } else {
        *last_slash = '\0';
        ret = iname(path, 0, &tparent);
        if (ret < 0) {
            free(path);
            return ret;
        }
    }

    struct mount *mount = tparent->inode->mounts;
    while (mount != NULL) {
        if (strcmp(mount->name, last_slash + 1) == 0) {
            free(path);
            drop_tnode(tparent);
            return -EEXIST;
        }

        mount = mount->next;
    }

    tparent->inode->i_op->lookup(tparent->inode, NULL);
    if (fs_lookup_in_cache(tparent->inode->dirent_cache, last_slash + 1) != NULL) {
        free(path);
        drop_tnode(tparent);
        return -EEXIST;
    }

    if (!tparent->inode->i_op->symlink) {
        free(path);
        drop_tnode(tparent);
        return -EINVAL;
    }

    debug_log("Adding symlink to: [ %s ]\n", tparent->name);

    int error = 0;
    struct inode *inode = tparent->inode->i_op->symlink(tparent, last_slash + 1, target, &error);
    if (inode == NULL) {
        free(path);
        drop_tnode(tparent);
        return error;
    }

    fs_put_dirent_cache(tparent->inode->dirent_cache, inode, last_slash + 1, strlen(last_slash + 1));
    drop_tnode(tparent);

    free(path);
    return 0;
}

int fs_link(const char *oldpath, const char *newpath) {
    struct tnode *target;

    int ret = iname(oldpath, 0, &target);
    if (ret < 0) {
        return ret;
    }

    if (target->inode->flags & FS_DIR) {
        return -EPERM;
    }

    char *path = malloc(strlen(newpath) + 1);
    strcpy(path, newpath);

    char *last_slash = strrchr(path, '/');
    struct tnode *tparent;

    if (last_slash == path) {
        tparent = bump_tnode(fs_root());
    } else if (last_slash == NULL) {
        tparent = bump_tnode(get_current_task()->process->cwd);
        last_slash = path - 1;
    } else {
        *last_slash = '\0';
        ret = iname(path, 0, &tparent);
        if (ret < 0) {
            free(path);
            return ret;
        }
    }

    struct mount *mount = tparent->inode->mounts;
    while (mount != NULL) {
        if (strcmp(mount->name, last_slash + 1) == 0) {
            free(path);
            drop_tnode(tparent);
            return -EEXIST;
        }

        mount = mount->next;
    }

    tparent->inode->i_op->lookup(tparent->inode, NULL);
    if (fs_lookup_in_cache(tparent->inode->dirent_cache, last_slash + 1) != NULL) {
        free(path);
        drop_tnode(tparent);
        return -EEXIST;
    }

    if (!tparent->inode->i_op->link) {
        free(path);
        drop_tnode(tparent);
        return -EINVAL;
    }

    debug_log("Adding hard link to: [ %s ]\n", tparent->name);

    ret = tparent->inode->i_op->link(tparent, last_slash + 1, target);
    if (ret < 0) {
        drop_tnode(tparent);
        return ret;
    }

    bump_inode_reference(target->inode);

    fs_put_dirent_cache(tparent->inode->dirent_cache, target->inode, last_slash + 1, strlen(last_slash + 1));

    drop_tnode(tparent);
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

    int fetched_ref_count = atomic_fetch_add(&desc.file->ref_count, 1);
    assert(fetched_ref_count > 0);

    // NOTE: the new descriptor reset the FD_CLOEXEC flag
    return (struct file_descriptor) { desc.file, 0 };
}

struct file_descriptor fs_dup_accross_fork(struct file_descriptor desc) {
    if (desc.file == NULL) {
        return (struct file_descriptor) { NULL, 0 };
    }

    int fetched_ref_count = atomic_fetch_add(&desc.file->ref_count, 1);
    assert(fetched_ref_count > 0);

    // NOTE: the new descriptor reset the FD_CLOEXEC flag
    return (struct file_descriptor) { desc.file, desc.fd_flags };
}

void init_vfs() {
    init_initrd();
    init_dev();
    init_ext2();
    init_pipe();
    init_tmpfs();
    init_procfs();

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

    error = fs_mount("", "/proc", "procfs");
    assert(error == 0);
}

char *get_tnode_path(struct tnode *tnode) {
    /* Check if root */
    if (!tnode->parent) {
        char *ret = malloc(2);
        strcpy(ret, "/");
        return ret;
    }

    ssize_t size = 15;
    char **name_buffer = malloc(size * sizeof(char **));

    ssize_t len = 1;
    ssize_t i;
    for (i = 0; tnode->parent != NULL; i++) {
        name_buffer[i] = tnode->name;
        len += strlen(tnode->name) + 1;
        tnode = tnode->parent;

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

    struct inode *inode = fs_file_inode(file);
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

    struct inode *inode = fs_file_inode(file);
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

    struct inode *inode = fs_file_inode(file);
    assert(inode);

    return inode->excetional_activity;
}

struct tnode *fs_get_tnode_for_file(struct file *file) {
    assert(file);
    return file->tnode;
}

ssize_t fs_do_read(char *buf, off_t offset, size_t n, const char *source, size_t source_max) {
    size_t to_read = MIN(n, source_max - offset);
    memcpy(buf, source + offset, to_read);
    return to_read;
}