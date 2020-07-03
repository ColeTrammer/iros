#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/proc/pid.h>
#include <kernel/proc/process_state.h>
#include <kernel/proc/task.h>
#include <kernel/sched/task_sched.h>
#include <kernel/util/hash_map.h>

static struct hash_map *queue_map;
static struct hash_map *pg_queue_map;
static struct hash_map *parent_queue_map;

HASH_DEFINE_FUNCTIONS(proc, struct proc_state_message_queue, pid_t, pid)
HASH_DEFINE_FUNCTIONS(pg, struct proc_state_message_queue, pid_t, pgid)
HASH_DEFINE_FUNCTIONS(parent, struct proc_state_message_queue, pid_t, ppid)

static void setup_queue_lists(struct proc_state_message_queue *queue) {
    struct proc_state_message_queue *pg_list = hash_get(pg_queue_map, &queue->pgid);
    if (pg_list == NULL) {
        hash_put(pg_queue_map, queue);
    } else {
        queue->pg_next = pg_list;
        hash_set(pg_queue_map, queue);
    }

    struct proc_state_message_queue *parent_list = hash_get(parent_queue_map, &queue->ppid);
    if (parent_list == NULL) {
        hash_put(parent_queue_map, queue);
    } else {
        queue->parent_next = parent_list;
        hash_set(parent_queue_map, queue);
    }
}

// Maybe need to lock the lists before removal on SMP
static void remove_from_queue_lists(struct proc_state_message_queue *queue) {
    struct proc_state_message_queue *pg_list = hash_get(pg_queue_map, &queue->pgid);
    assert(pg_list);

    if (pg_list->pg_next == NULL) {
        assert(pg_list == queue);
        hash_del(pg_queue_map, &queue->pgid);
    } else if (pg_list == queue) {
        hash_set(pg_queue_map, queue->pg_next);
    } else {
        while (pg_list->pg_next != queue) {
            pg_list = pg_list->pg_next;
        }

        pg_list->pg_next = queue->pg_next;
    }

    struct proc_state_message_queue *parent_list = hash_get(parent_queue_map, &queue->ppid);
    assert(parent_list);
    if (parent_list->parent_next == NULL) {
        assert(parent_list == queue);
        hash_del(parent_queue_map, &queue->ppid);
    } else if (parent_list == queue) {
        hash_set(parent_queue_map, queue->parent_next);
    } else {
        while (parent_list->parent_next != queue) {
            parent_list = parent_list->parent_next;
        }

        parent_list->parent_next = queue->parent_next;
    }
}

static struct proc_state_message_queue *ensure_queue(pid_t pid) {
    struct proc_state_message_queue *queue;
    if (!(queue = hash_get(queue_map, &pid))) {
        queue = malloc(sizeof(struct proc_state_message_queue));
        queue->pid = pid;
        struct process *proc = find_by_pid(pid);
        assert(proc);
        queue->pgid = proc->pgid;
        queue->ppid = proc->ppid;
        queue->start = NULL;
        queue->end = NULL;
        queue->pg_next = NULL;
        queue->parent_next = NULL;
        init_spinlock(&queue->lock);
        hash_put(queue_map, queue);
        setup_queue_lists(queue);
    }

    return queue;
}

// queue must be locked before calling this method
static void __free_queue(struct proc_state_message_queue *queue) {
    pid_t pid = queue->pid;
    pid_t ppid = queue->ppid;
    struct process *parent = find_by_pid(ppid);
    if (parent) {
        parent->times.tms_cutime = queue->times.tms_utime + queue->times.tms_cutime;
        parent->times.tms_cstime = queue->times.tms_stime + queue->times.tms_cstime;
    }

    hash_del(queue_map, &queue->pid);
    remove_from_queue_lists(queue);
    spin_unlock(&queue->lock);
    free(queue);

    // There is no longer anything associated with this pid, so we can free it
    free_pid(pid);
}

void proc_update_pgid(pid_t pid, pid_t pgid) {
    struct proc_state_message_queue *queue = hash_get(queue_map, &pid);
    if (queue == NULL) {
        return;
    }

    debug_log("Switching pgid: [ %d, %d ]\n", pid, pgid);
    if (queue->start) {
        // Removes it from old pg_list
        struct proc_state_message_queue *pg_list = hash_get(pg_queue_map, &queue->pgid);
        if (pg_list->pg_next == NULL) {
            assert(pg_list == queue);
            hash_del(pg_queue_map, &queue->pgid);
        } else if (pg_list == queue) {
            hash_set(pg_queue_map, queue->pg_next);
        } else {
            while (pg_list->pg_next != queue) {
                pg_list = pg_list->pg_next;
            }

            pg_list->pg_next = queue->pg_next;
        }

        // Add it to correct list
        queue->pgid = pgid;
        pg_list = hash_get(pg_queue_map, &queue->pgid);
        if (pg_list == NULL) {
            hash_put(pg_queue_map, queue);
        } else {
            queue->pg_next = pg_list;
            hash_set(pg_queue_map, queue);
        }
    }
}

struct proc_state_message *proc_create_message(int type, int data) {
    assert(type == STATE_EXITED || type == STATE_INTERRUPTED || type == STATE_STOPPED || type == STATE_CONTINUED);

    struct proc_state_message *m = malloc(sizeof(struct proc_state_message));
    m->type = type;
    m->data = data;
    m->next = NULL;
    return m;
}

void proc_add_message(pid_t pid, struct proc_state_message *m) {
    struct proc_state_message_queue *queue = ensure_queue(pid);

    struct process *process = find_by_pid(pid);
    assert(process);

    struct process *parent = find_by_pid(queue->ppid);

    // Ignore the message if requested
    if ((m->type == STATE_STOPPED || m->type == STATE_CONTINUED) &&
        (parent == NULL || parent->sig_state[SIGCHLD].sa_flags & (SA_NOCLDWAIT | SA_NOCLDSTOP))) {
        return;
    }

    if (m->type == STATE_EXITED || m->type == STATE_INTERRUPTED) {
        memcpy(&queue->times, &process->times, sizeof(struct tms));
    }

    if ((m->type == STATE_EXITED || m->type == STATE_INTERRUPTED) &&
        (parent == NULL || (parent->sig_state[SIGCHLD].sa_flags & SA_NOCLDWAIT))) {
        while (queue->start) {
            struct proc_state_message *next = queue->start->next;
            free(queue->start);
            queue->start = next;
        }

        __free_queue(queue);
        return;
    }

    spin_lock(&queue->lock);
    if (queue->start == NULL) {
        queue->start = queue->end = m;
    } else {
        queue->end->next = m;
        queue->end = m;
    }
    spin_unlock(&queue->lock);

    // Also gen SIGCHLD for the parent process
    proc_notify_parent(pid);
}

int proc_num_messages(pid_t pid) {
    struct proc_state_message_queue *queue = hash_get(queue_map, &pid);
    if (queue == NULL) {
        return 0;
    }

    int count = 0;

    spin_lock(&queue->lock);
    struct proc_state_message *m = queue->start;
    while (m) {
        count++;
        m = m->next;
    }
    spin_unlock(&queue->lock);

    return count;
}

pid_t proc_consume_message(pid_t pid, struct proc_state_message *m) {
    struct proc_state_message_queue *queue = ensure_queue(pid);
    bool empty = true;

    // FIXME: what if there's two processes waiting? One could free the queue before the other even takes the lock...
    spin_lock(&queue->lock);
    if (queue->start) {
        empty = false;
        memcpy(m, queue->start, sizeof(struct proc_state_message));

        struct proc_state_message *next = queue->start->next;
        free(queue->start);
        queue->start = next;

        // Clean up entire queue since there won't be any more messages
        if (m->type == STATE_EXITED || m->type == STATE_INTERRUPTED) {
            while (queue->start != NULL) {
                struct proc_state_message *next = queue->start->next;
                free(queue->start);
                queue->start = next;
            }
            __free_queue(queue);
            return pid;
        }
    }
    spin_unlock(&queue->lock);

    return empty ? 0 : pid;
}

int proc_num_messages_by_pg(pid_t pgid) {
    struct proc_state_message_queue *queue = hash_get(pg_queue_map, &pgid);

    if (queue == NULL) {
        return 0;
    }

    int count = 0;
    while (queue != NULL) {
        count += proc_num_messages(queue->pid);
        queue = queue->pg_next;
    }

    return count;
}

pid_t proc_consume_message_by_pg(pid_t pgid, struct proc_state_message *m) {
    struct proc_state_message_queue *queue = hash_get(pg_queue_map, &pgid);

    if (queue == NULL) {
        return 0;
    }

    while (queue != NULL) {
        if (queue->ppid == get_current_task()->process->pid) {
            pid_t ret = proc_consume_message(queue->pid, m);
            if (ret != 0) {
                return ret;
            }
        }

        queue = queue->pg_next;
    }

    return 0;
}

int proc_num_messages_by_parent(pid_t ppid) {
    struct proc_state_message_queue *queue = hash_get(parent_queue_map, &ppid);

    if (queue == NULL) {
        return 0;
    }

    int count = 0;
    while (queue != NULL) {
        count += proc_num_messages(queue->pid);
        queue = queue->parent_next;
    }

    return count;
}

pid_t proc_consume_message_by_parent(pid_t ppid, struct proc_state_message *m) {
    struct proc_state_message_queue *queue = hash_get(parent_queue_map, &ppid);

    if (queue == NULL) {
        return 0;
    }

    while (queue != NULL) {
        pid_t ret = proc_consume_message(queue->pid, m);
        if (ret != 0) {
            return ret;
        }

        queue = queue->parent_next;
    }

    return 0;
}

void init_proc_state() {
    queue_map = hash_create_hash_map(&proc_hash, &proc_equals, &proc_key);
    pg_queue_map = hash_create_hash_map(&pg_hash, &pg_equals, &pg_key);
    parent_queue_map = hash_create_hash_map(&parent_hash, &parent_equals, &parent_key);
}

pid_t proc_get_pgid(pid_t pid) {
    struct process *process = find_by_pid(pid);
    if (process != NULL) {
        return process->pgid;
    }

    struct proc_state_message_queue *queue = hash_get(queue_map, &pid);
    if (queue != NULL) {
        return queue->pgid;
    }

    return -ESRCH;
}
