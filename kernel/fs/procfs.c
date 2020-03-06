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

#define PROCFS_DEVICE_ID 4

#define PROCFS_FILE_MODE      (S_IFREG | 0444)
#define PROCFS_DIRECTORY_MODE (S_IFDIR | 0555)
#define PROCFS_SYMLINK_MODE   (S_IFLNK | 0777)

#define PROCFS_DYNAMIC_FLAG        (1UL)
#define PROCFS_IS_DYNAMIC(inode)   ((((uintptr_t)(inode)->private_data)) & PROCFS_DYNAMIC_FLAG)
#define PROCFS_MAKE_DYNAMIC(inode) ((inode)->private_data = (void *) ((((uintptr_t)(inode)->private_data)) | PROCFS_DYNAMIC_FLAG))

#define PROCFS_GET_FUNCTION(inode) ((procfs_function_t)(((uintptr_t)((inode)->private_data)) & ~(PROCFS_DYNAMIC_FLAG)))

static struct file_system fs;
static struct super_block super_block;

static spinlock_t inode_counter_lock = SPINLOCK_INITIALIZER;
static ino_t inode_counter = 1;

static struct file_system fs = { "procfs", 0, &procfs_mount, NULL, NULL };

static struct inode_operations procfs_i_op = { NULL, &procfs_lookup, &procfs_open,     NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                               NULL, NULL,           &procfs_read_all, NULL, NULL };

static struct inode_operations procfs_dir_i_op = { NULL, &procfs_lookup, &procfs_open, NULL, NULL, NULL, NULL, NULL,
                                                   NULL, NULL,           NULL,         NULL, NULL, NULL, NULL, NULL };

static struct file_operations procfs_f_op = { NULL, &procfs_read, NULL, NULL };

static struct file_operations procfs_dir_f_op = { NULL, NULL, NULL, NULL };

static struct inode *procfs_create_inode(struct tnode *tparent, mode_t mode, uid_t uid, gid_t gid, procfs_function_t function) {
    struct inode *inode = calloc(1, sizeof(struct inode));
    inode->flags = fs_mode_to_flags(mode);
    inode->super_block = &super_block;
    inode->access_time = inode->modify_time = inode->change_time = time_read_clock(CLOCK_REALTIME);
    inode->device = PROCFS_DEVICE_ID;
    inode->gid = gid;
    inode->uid = uid;
    inode->mode = mode;
    inode->i_op = S_ISDIR(mode) ? &procfs_dir_i_op : &procfs_i_op;
    inode->ref_count = 1;
    inode->parent = tparent;
    inode->private_data = function;
    init_spinlock(&inode->lock);

    spin_lock(&inode_counter_lock);
    inode->index = inode_counter++;
    spin_unlock(&inode_counter_lock);

    return inode;
}

static struct process *procfs_get_process(struct inode *inode) {
    if (PROCFS_IS_DYNAMIC(inode)) {
        return get_current_task()->process;
    }

    struct tnode *parent = inode->parent;
    pid_t pid = atoi(parent->name);
    return find_by_pid(pid);
}

static struct procfs_buffer procfs_get_data(struct inode *inode) {
    struct process *process = procfs_get_process(inode);

    procfs_function_t getter = PROCFS_GET_FUNCTION(inode);
    return getter(process);
}

static void procfs_cleanup_data(struct procfs_buffer data, struct inode *inode __attribute__((unused))) {
    free(data.buffer);
}

struct tnode *procfs_lookup(struct inode *inode, const char *name) {
    assert(inode);
    if (!name) {
        if (!(inode->flags & FS_DIR)) {
            struct procfs_buffer result = procfs_get_data(inode);
            inode->size = result.size;
            procfs_cleanup_data(result, inode);
        }

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

int procfs_read_all(struct inode *inode, void *buffer) {
    struct procfs_buffer data = procfs_get_data(inode);

    memcpy(buffer, data.buffer, data.size);

    procfs_cleanup_data(data, inode);
    return 0;
}

ssize_t procfs_read(struct file *file, off_t offset, void *buffer, size_t len) {
    struct inode *inode = fs_inode_get(file->device, file->inode_idenifier);
    assert(inode);

    struct procfs_buffer data = procfs_get_data(inode);

    size_t to_read = MIN(len, data.size - offset);
    memcpy(buffer, data.buffer + offset, to_read);

    procfs_cleanup_data(data, inode);
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

    struct inode *status_inode = procfs_create_inode(tparent, PROCFS_FILE_MODE, process->uid, process->gid, procfs_status);

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

    struct inode *inode = procfs_create_inode(t_root, PROCFS_DIRECTORY_MODE, process->uid, process->gid, NULL);

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

static struct procfs_buffer procfs_self(struct process *process) {
    char *buffer = aligned_alloc(PAGE_SIZE, PAGE_SIZE);
    size_t length = snprintf(buffer, PAGE_SIZE, "%d", process->pid);
    return (struct procfs_buffer) { buffer, length };
}

static void procfs_create_base_directory_structure(struct tnode *tparent) {
    struct inode *parent = tparent->inode;

    struct inode *self_inode = procfs_create_inode(tparent, PROCFS_SYMLINK_MODE, 0, 0, procfs_self);
    PROCFS_MAKE_DYNAMIC(self_inode);

    struct tnode *self_tnode = create_tnode("self", self_inode);
    parent->tnode_list = add_tnode(parent->tnode_list, self_tnode);
}

struct tnode *procfs_mount(struct file_system *current_fs, char *device_path) {
    assert(current_fs != NULL);
    assert(strlen(device_path) == 0);

    root = procfs_create_inode(NULL, PROCFS_DIRECTORY_MODE, 0, 0, NULL);
    t_root = create_root_tnode(root);

    super_block.device = root->device;
    super_block.op = NULL;
    super_block.root = t_root;
    super_block.block_size = PAGE_SIZE;

    current_fs->super_block = &super_block;

    procfs_create_base_directory_structure(t_root);

    return t_root;
}

void init_procfs() {
    load_fs(&fs);
}