#ifndef _KERNEL_FS_WATCH_H
#define _KERNEL_FS_WATCH_H 1

#include <kernel/util/list.h>

struct inode;
struct umessage_queue;

struct watcher {
    struct list_node list_for_inode;
    struct list_node list_for_queue;
    struct inode *inode;
    struct umessage_queue *queue;
    spinlock_t lock;
    int identifier;
    bool removed_from_inode;
};

struct umessage_watch_queue_private {
    struct list_node watchers;
};

static inline void fs_lock_watcher(struct watcher *watcher) {
    spin_lock(&watcher->lock);
}

static inline void fs_unlock_watcher(struct watcher *watcher) {
    spin_unlock(&watcher->lock);
}

#endif /* _KERNEL_FS_WATCH_H */
