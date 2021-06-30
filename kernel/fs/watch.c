#include <errno.h>
#include <stdlib.h>
#include <sys/umessage.h>

#include <kernel/fs/inode.h>
#include <kernel/fs/vfs.h>
#include <kernel/fs/watch.h>
#include <kernel/net/umessage.h>
#include <kernel/util/init.h>

static struct watcher *fs_create_watcher(int identifier, struct umessage_queue *queue, struct inode *inode) {
    struct watcher *watcher = calloc(1, sizeof(struct watcher));
    if (!watcher) {
        return NULL;
    }

    watcher->queue = queue;
    watcher->inode = inode;
    watcher->identifier = identifier;

    return watcher;
}

static int fs_create_and_register_watcher(int identifier, struct umessage_queue *queue, struct inode *inode) {
    struct watcher *watcher = fs_create_watcher(identifier, queue, inode);
    if (!watcher) {
        return -ENOMEM;
    }

    struct umessage_watch_queue_private *data = net_umessage_queue_private(queue);
    list_append(&data->watchers, &watcher->list_for_queue);

    fs_register_watcher(inode, watcher);
    return 0;
}

static void fs_free_watcher(struct watcher *watcher) {
    free(watcher);
}

static int watch_recv(struct umessage_queue *queue, const struct umessage *request) {
    struct umessage_watch_queue_private *data = net_umessage_queue_private(queue);

    switch (request->type) {
        case UMESSAGE_WATCH_ADD_PATH_REQUEST: {
            if (!UMESSAGE_WATCH_ADD_PATH_REQUEST_VALID(request, request->length)) {
                return -EINVAL;
            }

            const struct umessage_watch_add_path_request *req = (void *) request;

            struct tnode *tnode;
            int ret = iname(req->path, INAME_DONT_FOLLOW_TRAILING_SYMLINK, &tnode);
            if (ret < 0) {
                return ret;
            }

            ret = fs_create_and_register_watcher(req->identifier, queue, tnode->inode);

            drop_tnode(tnode);
            return ret;
        }
        case UMESSAGE_WATCH_REMOVE_WATCH_REQUEST: {
            if (!UMESSAGE_WATCH_REMOVE_WATCH_REQUEST_VALID(request, request->length)) {
                return -EINVAL;
            }

            const struct umessage_watch_remove_watch_request *req = (void *) request;
            list_for_each_entry_safe(&data->watchers, watcher, struct watcher, list_for_queue) {
                if (req->identifier == watcher->identifier) {
                    list_remove(&watcher->list_for_queue);
                    fs_unregister_watcher(watcher->inode, watcher);
                }
            }
            return 0;
        }
        default:
            return -EINVAL;
    }
}

static void watch_init(struct umessage_queue *queue) {
    struct umessage_watch_queue_private *data = net_umessage_queue_private(queue);
    init_list(&data->watchers);
}

static void watch_kill(struct umessage_queue *queue) {
    struct umessage_watch_queue_private *data = net_umessage_queue_private(queue);
    list_for_each_entry(&data->watchers, watcher, struct watcher, list_for_queue) {
        if (watcher->inode) {
            fs_unregister_watcher(watcher->inode, watcher);
        }
        fs_free_watcher(watcher);
    }
}

static struct umessage_category watch_category = {
    .category = UMESSAGE_WATCH,
    .request_type_count = UMESSAGE_INTERFACE_NUM_REQUESTS,
    .name = "UMessage Watch",
    .private_data_size = sizeof(struct umessage_watch_queue_private),
    .recv = watch_recv,
    .init = watch_init,
    .kill = watch_kill,
};

static void init_umessage_watch(void) {
    net_register_umessage_category(&watch_category);
}
INIT_FUNCTION(init_umessage_watch, net);
