#include <sys/param.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <kernel/fs/vfs.h>
#include <kernel/fs/inode.h>
#include <kernel/fs/inode_store.h>
#include <kernel/fs/initrd.h>
#include <kernel/fs/file_system.h>
#include <kernel/mem/vm_region.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/hal/output.h>

static struct file_system *file_systems;

void init_vfs() {
    init_initrd();

    /* Mount INITRD */
    file_systems->mount(file_systems);
}

struct file *fs_open(const char *file_name) {
    if (file_name == NULL) {
        return NULL;
    }

    if (file_name[0] != '/') {
        return NULL;
    }

    struct tnode *t_root = file_systems->super_block->root;
    assert(t_root != NULL);
    assert(t_root->inode != NULL);
    assert(t_root->inode->i_op != NULL);
    assert(t_root->inode->i_op->lookup != NULL);
    
    struct tnode *tnode = t_root->inode->i_op->lookup(t_root->inode, file_name + 1);
    assert(tnode != NULL);

    debug_log("File Opened: [ %s ]\n", file_name);
    struct inode *inode = tnode->inode;
    fs_inode_put(inode);
    
    return inode->i_op->open(tnode->inode);
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