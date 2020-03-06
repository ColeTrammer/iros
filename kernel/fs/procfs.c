#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include <kernel/fs/file.h>
#include <kernel/fs/file_system.h>
#include <kernel/fs/inode.h>
#include <kernel/fs/inode_store.h>
#include <kernel/fs/procfs.h>
#include <kernel/fs/super_block.h>
#include <kernel/fs/vfs.h>
#include <kernel/hal/output.h>
#include <kernel/hal/timer.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/mem/vm_region.h>
#include <kernel/proc/task.h>
#include <kernel/time/clock.h>
#include <kernel/util/spinlock.h>

static struct file_system fs;
static struct super_block super_block;

static spinlock_t inode_counter_lock = SPINLOCK_INITIALIZER;
static ino_t inode_counter = 1;

static struct file_system fs = { "procfs", 0, &procfs_mount, NULL, NULL };

static struct inode_operations procfs_i_op = { NULL, NULL, &procfs_open, NULL, NULL, NULL, NULL, NULL,
                                               NULL, NULL, NULL,         NULL, NULL, NULL, NULL, NULL };

static struct inode_operations procfs_dir_i_op = { NULL, &procfs_lookup, &procfs_open, NULL, NULL, NULL, NULL, NULL,
                                                   NULL, NULL,           NULL,         NULL, NULL, NULL, NULL, NULL };

static struct file_operations procfs_f_op = { NULL, &procfs_read, NULL, NULL };

static struct file_operations procfs_dir_f_op = { NULL, NULL, NULL, NULL };

struct tnode *procfs_lookup(struct inode *inode, const char *name) {
    if (inode == NULL || inode->tnode_list == NULL || name == NULL) {
        return NULL;
    }

    struct tnode_list *list = inode->tnode_list;
    while (list != NULL) {
        if (strcmp(list->tnode->name, name) == 0) {
            return list->tnode;
        }
        list = list->next;
    }

    return NULL;
}

struct file *procfs_open(struct inode *inode, int flags, int *error) {
    assert(!(flags & O_RDWR));

    struct file *file = calloc(1, sizeof(struct file));
    file->device = inode->device;
    file->f_op = (inode->flags & FS_DIR) ? &procfs_dir_f_op : &procfs_f_op;
    file->flags = inode->flags;
    file->inode_idenifier = inode->index;
    file->length = inode->size;
    file->position = 0;
    file->start = 0;
    file->abilities = 0;
    file->ref_count = 0;

    *error = 0;
    return file;
}

static struct process *procfs_get_process(struct inode *inode) {
    struct tnode *parent = inode->parent;
    pid_t pid = atoi(parent->name);
    return find_by_pid(pid);
}

ssize_t procfs_read(struct file *file, off_t offset, void *buffer, size_t len) {
    struct inode *inode = fs_inode_get(file->device, file->inode_idenifier);
    assert(inode);

    struct process *process = procfs_get_process(inode);

    procfs_function_t getter = inode->private_data;
    assert(getter);

    struct procfs_buffer data = getter(process);

    size_t to_read = MIN(len, data.size - offset);
    memcpy(buffer, data.buffer + offset, to_read);

    free(data.buffer);
    return to_read;
}

static struct procfs_buffer procfs_status(struct process *process) {
    char *buffer = aligned_alloc(PAGE_SIZE, PAGE_SIZE);
    size_t length = snprintf(buffer, PAGE_SIZE,
                             "PID: %d\n"
                             "UID: %hu\n"
                             "GID: %hu\n"
                             "PPID: %d\n"
                             "UMASK: %04o\n"
                             "EUID: %hu\n"
                             "EGID: %hu\n"
                             "PGID: %d\n"
                             "SID: %d\n",
                             process->pid, process->uid, process->gid, process->ppid, process->umask, process->euid, process->egid,
                             process->pgid, process->sid);
    return (struct procfs_buffer) { buffer, length };
}

static struct inode *root;
static struct tnode *t_root;

static void procfs_create_process_directory_structure(struct tnode *tparent, struct process *process __attribute__((unused))) {
    struct inode *parent = tparent->inode;

    struct inode *status_inode = calloc(1, sizeof(struct inode));
    status_inode->flags = FS_FILE;
    status_inode->super_block = parent->super_block;
    status_inode->access_time = status_inode->modify_time = status_inode->change_time = time_read_clock(CLOCK_REALTIME);
    status_inode->device = parent->device;
    status_inode->mode = S_IFREG | 0444;
    status_inode->i_op = &procfs_i_op;
    status_inode->ref_count = 1;
    status_inode->parent = tparent;
    status_inode->private_data = &procfs_status;
    init_spinlock(&status_inode->lock);
    spin_lock(&inode_counter_lock);
    status_inode->index = inode_counter++;
    spin_unlock(&inode_counter_lock);

    struct tnode *status_tnode = create_tnode("status", status_inode);
    parent->tnode_list = add_tnode(parent->tnode_list, status_tnode);
}

static void procfs_destroy_process_directory_structure(struct inode *parent) {
    struct tnode_list *tnode_list_node = parent->tnode_list;
    while (tnode_list_node) {
        struct tnode *tnode = tnode_list_node->tnode;
        tnode_list_node = remove_tnode(tnode_list_node, tnode);
        drop_tnode(tnode);
    }
}

void procfs_register_process(struct process *process) {
    assert(root);

    struct inode *inode = calloc(1, sizeof(struct inode));
    inode->flags = FS_DIR;
    inode->super_block = root->super_block;
    inode->access_time = inode->modify_time = inode->change_time = time_read_clock(CLOCK_REALTIME);
    inode->device = root->device;
    inode->gid = process->gid;
    inode->uid = process->uid;
    inode->mode = S_IFDIR | 0555;
    inode->i_op = &procfs_dir_i_op;
    inode->ref_count = 1;
    inode->parent = t_root;
    init_spinlock(&inode->lock);
    spin_lock(&inode_counter_lock);
    inode->index = inode_counter++;
    spin_unlock(&inode_counter_lock);

    char pid_string[16];
    snprintf(pid_string, sizeof(pid_string) - 1, "%d", process->pid);
    struct tnode *parent = create_tnode(pid_string, inode);

    procfs_create_process_directory_structure(parent, process);

    spin_lock(&root->lock);
    root->tnode_list = add_tnode(root->tnode_list, parent);
    spin_unlock(&root->lock);
}

void procfs_unregister_process(struct process *process) {
    assert(root);

    char pid_string[16];
    snprintf(pid_string, sizeof(pid_string) - 1, "%d", process->pid);
    struct tnode *tnode = find_tnode(root->tnode_list, pid_string);
    assert(tnode);

    spin_lock(&root->lock);
    root->tnode_list = remove_tnode(root->tnode_list, tnode);
    spin_unlock(&root->lock);

    procfs_destroy_process_directory_structure(tnode->inode);
    drop_tnode(tnode);
}

struct tnode *procfs_mount(struct file_system *current_fs, char *device_path) {
    assert(current_fs != NULL);
    assert(strlen(device_path) == 0);

    root = calloc(1, sizeof(struct inode));
    t_root = create_root_tnode(root);

    root->device = 4;
    root->flags = FS_DIR;
    root->i_op = &procfs_dir_i_op;
    root->index = inode_counter++;
    init_spinlock(&root->lock);
    root->mode = S_IFDIR | 0777;
    root->mounts = NULL;
    root->private_data = NULL;
    root->size = 0;
    root->super_block = &super_block;
    root->tnode_list = NULL;
    root->ref_count = 1;
    root->readable = true;
    root->writeable = true;
    root->access_time = root->change_time = root->modify_time = time_read_clock(CLOCK_REALTIME);

    super_block.device = root->device;
    super_block.op = NULL;
    super_block.root = t_root;
    super_block.block_size = PAGE_SIZE;

    current_fs->super_block = &super_block;

    return t_root;
}

void init_procfs() {
    load_fs(&fs);
}