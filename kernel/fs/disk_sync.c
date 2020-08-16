#include <string.h>

#include <kernel/fs/disk_sync.h>
#include <kernel/fs/inode.h>
#include <kernel/fs/super_block.h>
#include <kernel/fs/vfs.h>
#include <kernel/hal/output.h>
#include <kernel/proc/task.h>
#include <kernel/proc/wait_queue.h>
#include <kernel/util/list.h>
#include <kernel/util/mutex.h>

#define DISK_SYNC_DEBUG

static mutex_t queue_lock = MUTEX_INITIALIZER;
static struct wait_queue wait_queue = WAIT_QUEUE_INITIALIZER;
static struct list_node inode_queue = INIT_LIST(inode_queue);
static struct list_node super_block_queue = INIT_LIST(super_block_queue);

void fs_mark_inode_as_dirty(struct inode *inode) {
    if (inode->dirty || !inode->i_op->sync) {
        return;
    }

#ifdef DISK_SYNC_DEBUG
    debug_log("Enqueuing inode: [ %ld, %llu ]\n", inode->fsid, inode->index);
#endif /* DISK_SYNC_DEBUG */

    bump_inode_reference(inode);

    mutex_lock(&queue_lock);
    inode->dirty = true;
    list_append(&inode_queue, &inode->dirty_inodes);
    wake_up_all(&wait_queue);
    mutex_unlock(&queue_lock);
}

void fs_mark_super_block_as_dirty(struct super_block *sb) {
    if (sb->dirty || !sb->op->sync) {
        return;
    }

#ifdef DISK_SYNC_DEBUG
    debug_log("Enqueuing super block: [ %ld ]\n", sb->fsid);
#endif /* DISK_SYNC_DEBUG */

    mutex_lock(&queue_lock);
    sb->dirty = true;
    list_append(&super_block_queue, &sb->dirty_super_blocks);
    wake_up_all(&wait_queue);

    mutex_unlock(&queue_lock);
}

static void do_disk_sync(void) {
    for (;;) {
        struct super_block *sb_to_sync = NULL;
        struct inode *inode_to_sync = NULL;

        mutex_lock(&queue_lock);
        if (!list_is_empty(&super_block_queue)) {
            sb_to_sync = list_first_entry(&super_block_queue, struct super_block, dirty_super_blocks);
            list_remove(&sb_to_sync->dirty_super_blocks);
        } else if (!list_is_empty(&inode_queue)) {
            inode_to_sync = list_first_entry(&inode_queue, struct inode, dirty_inodes);
            list_remove(&inode_to_sync->dirty_inodes);
        } else {
            mutex_unlock(&queue_lock);
            wait_on(&wait_queue);
            continue;
        }
        mutex_unlock(&queue_lock);

        if (sb_to_sync) {
#ifdef DISK_SYNC_DEBUG
            debug_log("Syncing super block: [ %ld ]\n", sb_to_sync->fsid);
#endif /* DISK_SYNC_DEBUG */

            mutex_lock(&sb_to_sync->super_block_lock);
            if (sb_to_sync->dirty) {
                int ret = sb_to_sync->op->sync(sb_to_sync);
                if (ret) {
                    debug_log("ERROR: failed to sync super block: [ %ld, %s ]\n", sb_to_sync->fsid, strerror(-ret));
                }
            }
            mutex_unlock(&sb_to_sync->super_block_lock);
        } else if (inode_to_sync) {
#ifdef DISK_SYNC_DEBUG
            debug_log("Syncing inode: [ %ld, %llu ]\n", inode_to_sync->fsid, inode_to_sync->index);
#endif /* DISK_SYNC_DEBUG */

            mutex_lock(&inode_to_sync->lock);
            if (inode_to_sync->dirty) {
                inode_to_sync->dirty = false;
                int ret = inode_to_sync->i_op->sync(inode_to_sync);
                if (ret) {
                    debug_log("ERROR: failed to sync inode: [ %ld, %llu, %s ]\n", inode_to_sync->fsid, inode_to_sync->index,
                              strerror(-ret));
                }
            }
            mutex_unlock(&inode_to_sync->lock);
            drop_inode_reference(inode_to_sync);
        }
    }
}

void init_disk_sync_task(void) {
    load_kernel_task((uintptr_t) &do_disk_sync, "disk_sync");
}
