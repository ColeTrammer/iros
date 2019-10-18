#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>

#include <kernel/proc/pid.h>
#include <kernel/proc/process.h>
#include <kernel/proc/process_state.h>
#include <kernel/sched/process_sched.h>
#include <kernel/util/hash_map.h>

static struct hash_map *queue_map;

static int proc_hash(void *index, int num_buckets) {
    return *((pid_t*) index) % num_buckets;
}

static int proc_equals(void *i1, void *i2) {
    return *((pid_t*) i1) == *((pid_t*) i2);
}

static void *proc_key(void *block_group) {
    return &((struct proc_state_message_queue*) block_group)->pid;
}

static struct proc_state_message_queue *ensure_queue(pid_t pid) {
    struct proc_state_message_queue *queue;
    if (!(queue = hash_get(queue_map, &pid))) {
        queue = malloc(sizeof(struct proc_state_message_queue));
        queue->pid = pid;
        queue->start = NULL;
        queue->end = NULL;
        init_spinlock(&queue->lock);
        hash_put(queue_map, queue);
    }

    return queue;
}

// queue must be locked before calling this method
static void __free_queue(struct proc_state_message_queue *queue) {
    pid_t pid = queue->pid;

    hash_del(queue_map, &queue->pid);
    spin_unlock(&queue->lock);
    free(queue);

    // There is no longer anything associated with this pid, so we can free it
    free_pid(pid);
}

struct proc_state_message *proc_create_message(int type, int data) {
    assert(type == STATE_EXITED || type == STATE_INTERRUPTED || type == STATE_STOPPED);

    struct proc_state_message *m = malloc(sizeof(struct proc_state_message));
    m->type = type;
    m->data = data;
    m->next = NULL;
    return m;
}

void proc_add_message(pid_t pid, struct proc_state_message *m) {
    struct proc_state_message_queue *queue = ensure_queue(pid);

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

bool proc_consume_message(pid_t pid, struct proc_state_message *m) {
    struct proc_state_message_queue *queue = ensure_queue(pid);
    bool empty = true;

    spin_lock(&queue->lock);

    if (queue->start) {
        empty = false;
        memcpy(m, queue->start, sizeof(struct proc_state_message));

        struct proc_state_message *next = queue->start->next;
        free(queue->start);
        queue->start = next;

        // Clean up entire queue since there won't be any more messages
        if (m->type == STATE_EXITED) {
            assert(queue->start == NULL);
            __free_queue(queue);
            return true;
        }
    }

    spin_unlock(&queue->lock);
    return !empty;
}

void init_proc_state() {
    queue_map = hash_create_hash_map(&proc_hash, &proc_equals, &proc_key);
}