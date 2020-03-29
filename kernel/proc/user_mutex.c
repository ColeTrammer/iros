#include <assert.h>
#include <stdlib.h>

#include <kernel/hal/output.h>
#include <kernel/mem/page.h>
#include <kernel/proc/task.h>
#include <kernel/proc/user_mutex.h>
#include <kernel/util/hash_map.h>

#define USER_MUTEX_DEBUG

static struct hash_map *map;

HASH_DEFINE_FUNCTIONS(_, struct user_mutex, uintptr_t, phys_addr)

struct user_mutex *__create(uintptr_t *phys_addr_p) {
    struct user_mutex *m = malloc(sizeof(struct user_mutex));
    init_wait_queue(&m->wait_queue);
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

    spin_lock(&m->wait_queue.lock);
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
        spin_lock(&m->wait_queue.lock);
        if (__wait_queue_is_empty(&m->wait_queue)) {
            __write_value(&args);
            hash_del(map, &m->phys_addr);
            spin_unlock(&m->wait_queue.lock);

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
    spin_unlock(&um->wait_queue.lock);
}

void add_to_user_mutex_queue(struct user_mutex *m, struct task *task) {
    __wait_queue_enqueue_task(&m->wait_queue, task);

#ifdef USER_MUTEX_DEBUG
    debug_log("Adding to queue: [ %d:%d ]\n", task->process->pid, task->tid);
#endif /* USER_MUTEX_DEBUG */
}

void wake_user_mutex(struct user_mutex *m, int to_wake, int *to_place) {
    __wake_up_n(&m->wait_queue, to_wake);

    // This means there are no waiters left
    if (to_place && __wait_queue_is_empty(&m->wait_queue)) {
        debug_log("No Waiters on mutex\n");
        *to_place &= ~MUTEX_WAITERS;
    }
}

void user_mutex_wait_on(struct user_mutex *m) {
    wait_on(&m->wait_queue);
}

void init_user_mutexes() {
    map = hash_create_hash_map(__hash, __equals, __key);
}