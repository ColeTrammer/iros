#include <assert.h>
#include <stdlib.h>

#include <kernel/hal/output.h>
#include <kernel/mem/page.h>
#include <kernel/proc/task.h>
#include <kernel/proc/user_mutex.h>
#include <kernel/util/hash_map.h>

#define USER_MUTEX_DEBUG

static struct hash_map *map;

int __hash(void *k, int num_buckets) {
    return *((uintptr_t *) k) % num_buckets;
}

int __equals(void *k1, void *k2) {
    return *((uintptr_t *) k1) == *((uintptr_t *) k2);
}

void *__key(void *m) {
    return &((struct user_mutex *) m)->phys_addr;
}

struct user_mutex *__create(uintptr_t *phys_addr_p) {
    struct user_mutex *m = malloc(sizeof(struct user_mutex));
    init_spinlock(&m->lock);
    m->next_to_wake_up = NULL;
    m->phys_addr = *phys_addr_p;

    struct process *process = get_current_task()->process;
    spin_lock(&process->lock);

    m->next = process->used_user_mutexes;
    process->used_user_mutexes = m;

    spin_unlock(&process->lock);

#ifdef USER_MUTEX_DEBUG
    debug_log("Creating mutex: [ %#.16lX ]\n", *phys_addr_p);
#endif /* USER_MUTEX_DEBUG */
    return m;
}

struct user_mutex *get_user_mutex_locked(unsigned int *addr) {
    uintptr_t phys_addr = get_phys_addr((uintptr_t) addr);
    struct user_mutex *m = hash_put_if_not_present(map, &phys_addr, (void *(*) (void *) ) __create);

    spin_lock(&m->lock);
    return m;
}

struct __write_value_args {
    unsigned int *addr;
    unsigned int value;
};

static void __write_value(struct __write_value_args *args) {
    *(args->addr) = args->value;
}

struct user_mutex *get_user_mutex_locked_with_waiters_or_else_write_value(unsigned int *addr, int value) {
    uintptr_t phys_addr = get_phys_addr((uintptr_t) addr);

    struct __write_value_args args = { addr, (unsigned int) value };
    struct user_mutex *m = hash_get_or_else_do(map, &phys_addr, (void (*)(void *)) __write_value, &args);
    if (m) {
        spin_lock(&m->lock);
        if (m->next_to_wake_up == NULL) {
            __write_value(&args);
            hash_del(map, &m->phys_addr);
            spin_unlock(&m->lock);

#ifdef USER_MUTEX_DEBUG
            debug_log("Destroying mutex: [ %#.16lX ]\n", (uintptr_t) addr);
#endif /* USER_MUTEX_DEBUG */

            struct process *process = get_current_task()->process;
            spin_lock(&process->lock);

            struct user_mutex *mu = process->used_user_mutexes;
            if (m == mu) {
                process->used_user_mutexes = m->next;
            } else {
                while (mu && mu->next != m) {
                    mu = mu->next;
                }
                assert(mu);
                mu->next = m->next;
            }

            spin_unlock(&process->lock);

            free(m);
            return NULL;
        }
    }
#ifdef USER_MUTEX_DEBUG
    else {
        debug_log("Failed to find mutex: [ %#.16lX ]\n", phys_addr);
    }
#endif /* USER_MUTEX_DEBUG */
    return m;
}

void unlock_user_mutex(struct user_mutex *um) {
    spin_unlock(&um->lock);
}

void add_to_user_mutex_queue(struct user_mutex *m, struct task *task) {
    struct task **link = &m->next_to_wake_up;
    while (*link) {
        link = &(*link)->user_mutex_waiting_queue_next;
    }

    task->user_mutex_waiting_queue_next = NULL;
    *link = task;

#ifdef USER_MUTEX_DEBUG
    debug_log("Adding to queue: [ %d:%d ]\n", task->process->pid, task->tid);
#endif /* USER_MUTEX_DEBUG */
}

void wake_user_mutex(struct user_mutex *m, int to_wake) {
    for (int i = 0; i < to_wake; i++) {
        struct task *to_wake = m->next_to_wake_up;
        if (to_wake == NULL) {
            break;
        }

        m->next_to_wake_up = to_wake->user_mutex_waiting_queue_next;

#ifdef USER_MUTEX_DEBUG
        debug_log("Waking up: [ %d:%d ]\n", to_wake->process->pid, to_wake->tid);
#endif /* USER_MUTEX_DEBUG */

        to_wake->sched_state = RUNNING_UNINTERRUPTIBLE;
    }
}

void init_user_mutexes() {
    map = hash_create_hash_map(__hash, __equals, __key);
}