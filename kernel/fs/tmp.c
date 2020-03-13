#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/param.h>

#include <kernel/fs/cached_dirent.h>
#include <kernel/fs/file.h>
#include <kernel/fs/file_system.h>
#include <kernel/fs/inode.h>
#include <kernel/fs/inode_store.h>
#include <kernel/fs/super_block.h>
#include <kernel/fs/tmp.h>
#include <kernel/fs/vfs.h>
#include <kernel/hal/output.h>
#include <kernel/hal/timer.h>
#include <kernel/mem/inode_vm_object.h>
#include <kernel/mem/page.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/mem/vm_region.h>
#include <kernel/proc/task.h>
#include <kernel/time/clock.h>
#include <kernel/util/spinlock.h>

static struct file_system fs;

static spinlock_t inode_count_lock = SPINLOCK_INITIALIZER;
static ino_t inode_counter = 1;
static dev_t tmp_fs_id = 0x210;

static struct file_system fs = { "tmpfs", 0, &tmp_mount, NULL, NULL };

static struct super_block_operations s_op = { &tmp_rename };

static struct inode_operations tmp_i_op = { NULL,       &tmp_lookup,   &tmp_open,   NULL,
                                            NULL,       NULL,          &tmp_unlink, NULL,
                                            &tmp_chmod, &tmp_chown,    &tmp_mmap,   NULL,
                                            NULL,       &tmp_read_all, &tmp_utimes, &tmp_on_inode_destruction };

static struct inode_operations tmp_dir_i_op = { &tmp_create, &tmp_lookup, &tmp_open, NULL, NULL, &tmp_mkdir, NULL,        &tmp_rmdir,
                                                &tmp_chmod,  &tmp_chown,  NULL,      NULL, NULL, NULL,       &tmp_utimes, NULL };

static struct file_operations tmp_f_op = { NULL, &tmp_read, &tmp_write, NULL };

static struct file_operations tmp_dir_f_op = { NULL, NULL, NULL, NULL };

static ino_t get_next_tmp_index() {
    spin_lock(&inode_count_lock);
    ino_t next = inode_counter++;
    spin_unlock(&inode_count_lock);
    return next;
}

struct inode *tmp_create(struct tnode *tparent, const char *name, mode_t mode, int *error) {
    assert(tparent);
    assert(tparent->inode->flags & FS_DIR);
    assert(name);

    debug_log("Tmp create: [ %s ]\n", name);

    struct inode *inode = calloc(1, sizeof(struct inode));
    if (inode == NULL) {
        *error = ENOMEM;
        return NULL;
    }

    struct tmp_data *data = calloc(1, sizeof(struct tmp_data));
    data->owner = get_current_task()->process->pid;

    inode->i_op = &tmp_i_op;
    inode->index = get_next_tmp_index();
    init_spinlock(&inode->lock);
    inode->mode = mode;
    inode->uid = get_current_task()->process->uid;
    inode->gid = get_current_task()->process->gid;
    inode->mounts = NULL;
    inode->device = tparent->inode->device;
    inode->private_data = data;
    inode->ref_count = 1;
    inode->super_block = tparent->inode->super_block;
    inode->flags = S_ISREG(mode) ? FS_FILE : S_ISSOCK(mode) ? FS_SOCKET : 0;
    inode->writeable = true;
    inode->readable = true;
    inode->access_time = inode->change_time = inode->modify_time = time_read_clock(CLOCK_REALTIME);

    return inode;
}

struct inode *tmp_lookup(struct inode *inode, const char *name) {
    if (inode == NULL || name == NULL) {
        return NULL;
    }

    return fs_lookup_in_cache(inode->dirent_cache, name);
}

struct file *tmp_open(struct inode *inode, int flags, int *error) {
    (void) flags;

    struct file *file = calloc(1, sizeof(struct file));
    if (file == NULL) {
        *error = -ENOMEM;
        return NULL;
    }

    file->f_op = (inode->flags & FS_DIR) ? &tmp_dir_f_op : &tmp_f_op;
    file->flags = inode->flags;
    init_spinlock(&file->lock);
    file->device = inode->device;
    file->inode_idenifier = inode->index;
    return file;
}

ssize_t tmp_read(struct file *file, off_t offset, void *buffer, size_t len) {
    struct inode *inode = fs_inode_get(file->device, file->inode_idenifier);
    assert(inode);

    struct tmp_data *data = inode->private_data;
    assert(data);

    spin_lock(&inode->lock);
    size_t to_read = MIN(len, inode->size - offset);

    if (to_read == 0) {
        spin_unlock(&inode->lock);
        return 0;
    }

    memcpy(buffer, data->contents + offset, to_read);
    offset += to_read;

    inode->access_time = time_read_clock(CLOCK_REALTIME);

    spin_unlock(&inode->lock);
    return (ssize_t) to_read;
}

ssize_t tmp_write(struct file *file, off_t offset, const void *buffer, size_t len) {
    if (len == 0) {
        return len;
    }

    struct inode *inode = fs_inode_get(file->device, file->inode_idenifier);
    assert(inode);

    struct tmp_data *data = inode->private_data;
    assert(data);

    spin_lock(&inode->lock);
    if (data->contents == NULL) {
        data->max = ((offset + len + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE;
        data->contents = aligned_alloc(PAGE_SIZE, data->max);
        assert(data->contents);
        assert(((uintptr_t) data->contents) % PAGE_SIZE == 0);
    }

    inode->size = MAX(inode->size, offset + len);
    if (inode->size > data->max) {
        data->max = ((inode->size + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE;
        char *save = data->contents;
        data->contents = aligned_alloc(PAGE_SIZE, data->max);
        assert(((uintptr_t) data->contents) % PAGE_SIZE == 0);
        memcpy(data->contents, save, inode->size);
        free(save);
    }

    memcpy(data->contents + offset, buffer, len);

    inode->modify_time = time_read_clock(CLOCK_REALTIME);

    spin_unlock(&inode->lock);
    return (ssize_t) len;
}

struct inode *tmp_mkdir(struct tnode *tparent, const char *name, mode_t mode, int *error) {
    assert(name);

    struct inode *inode = calloc(1, sizeof(struct inode));
    inode->i_op = &tmp_dir_i_op;
    inode->index = get_next_tmp_index();
    init_spinlock(&inode->lock);
    inode->mode = mode;
    inode->uid = get_current_task()->process->uid;
    inode->gid = get_current_task()->process->gid;
    inode->ref_count = 1;
    inode->super_block = tparent->inode->super_block;
    inode->flags = FS_DIR;
    inode->device = tparent->inode->device;
    inode->writeable = true;
    inode->readable = true;
    tparent->inode->modify_time = inode->access_time = inode->modify_time = inode->change_time = time_read_clock(CLOCK_REALTIME);
    inode->dirent_cache = fs_create_dirent_cache();

    *error = 0;
    return inode;
}

int tmp_unlink(struct tnode *tnode) {
    (void) tnode;
    return 0;
}

int tmp_rmdir(struct tnode *tnode) {
    (void) tnode;
    return 0;
}

int tmp_chmod(struct inode *inode, mode_t mode) {
    inode->mode = mode;
    inode->modify_time = inode->access_time = time_read_clock(CLOCK_REALTIME);
    return 0;
}

int tmp_chown(struct inode *inode, uid_t uid, gid_t gid) {
    inode->uid = uid;
    inode->gid = gid;
    inode->modify_time = inode->access_time = time_read_clock(CLOCK_REALTIME);
    return 0;
}

int tmp_utimes(struct inode *inode, const struct timeval *times) {
    inode->access_time = (struct timespec) { .tv_sec = times[0].tv_sec, .tv_nsec = times[0].tv_usec * 1000 };
    inode->modify_time = (struct timespec) { .tv_sec = times[1].tv_sec, .tv_nsec = times[1].tv_usec * 1000 };
    return 0;
}

int tmp_rename(struct tnode *tnode, struct tnode *new_parent, const char *new_name) {
    (void) tnode;
    (void) new_name;

    new_parent->inode->modify_time = time_read_clock(CLOCK_REALTIME);

    return 0;
}

intptr_t tmp_mmap(void *addr, size_t len, int prot, int flags, struct inode *inode, off_t offset) {
    if (offset != 0 || !(flags & MAP_SHARED) || len > inode->size || len == 0) {
        return -EINVAL;
    }

    struct tmp_data *data = inode->private_data;
    if (!inode->vm_object) {
        inode->vm_object = vm_create_direct_inode_object(inode, data->contents);
    } else {
        bump_vm_object(inode->vm_object);
    }

    struct vm_region *region = map_region(addr, len, prot, VM_DEVICE_MEMORY_MAP_DONT_FREE_PHYS_PAGES);
    region->vm_object = inode->vm_object;
    region->vm_object_offset = 0;

    int ret = vm_map_region_with_object(region);
    if (ret < 0) {
        return (intptr_t) ret;
    }

    return (intptr_t) region->start;
}

int tmp_read_all(struct inode *inode, void *buffer) {
    struct tmp_data *data = inode->private_data;

    spin_lock(&inode->lock);
    memcpy(buffer, data->contents, inode->size);
    inode->access_time = time_read_clock(CLOCK_REALTIME);
    spin_unlock(&inode->lock);

    return 0;
}

void tmp_on_inode_destruction(struct inode *inode) {
    struct tmp_data *data = inode->private_data;
    if (data) {
        free(data->contents);
        free(data);
    }
}

struct inode *tmp_mount(struct file_system *current_fs, char *device_path) {
    assert(current_fs != NULL);
    assert(strlen(device_path) == 0);

    struct super_block *sb = calloc(1, sizeof(struct super_block));
    sb->block_size = PAGE_SIZE;
    sb->dev_file = NULL;
    sb->device = tmp_fs_id++;
    sb->op = &s_op;
    sb->private_data = NULL;
    init_spinlock(&sb->super_block_lock);

    struct inode *root = calloc(1, sizeof(struct inode));

    root->device = sb->device;
    root->flags = FS_DIR;
    root->i_op = &tmp_dir_i_op;
    spin_lock(&inode_count_lock);
    root->index = inode_counter++;
    spin_unlock(&inode_count_lock);
    init_spinlock(&root->lock);
    root->mode = S_IFDIR | 0777;
    root->mounts = NULL;
    root->private_data = NULL;
    root->size = 0;
    root->super_block = sb;
    root->ref_count++;
    root->readable = true;
    root->writeable = true;
    root->access_time = root->change_time = root->modify_time = time_read_clock(CLOCK_REALTIME);
    root->dirent_cache = fs_create_dirent_cache();

    sb->root = root;

    current_fs->super_block = sb;

    return root;
}

void init_tmpfs() {
    load_fs(&fs);
}