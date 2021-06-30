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
    int identifier;
};

struct umessage_watch_queue_private {
    struct list_node watchers;
};

#endif /* _KERNEL_FS_WATCH_H */
