#ifndef _KERNEL_PROC_USER_MUTEX_H
#define _KERNEL_PROC_USER_MUTEX_H 1

#include <stdint.h>

#include <kernel/proc/wait_queue.h>
#include <kernel/util/hash_map.h>

struct user_mutex {
    struct user_mutex *next;
    uintptr_t phys_addr;
    struct wait_queue wait_queue;
    struct hash_entry hash;
};

struct user_mutex *get_user_mutex_locked(unsigned int *addr);
struct user_mutex *get_user_mutex_locked_with_waiters_or_else_write_value(unsigned int *addr, int value);
void free_user_mutex(struct user_mutex *m);
void unlock_user_mutex(struct user_mutex *um);
void add_to_user_mutex_queue(struct user_mutex *m, struct task *task);
void wake_user_mutex(struct user_mutex *m, int to_wake, int *to_place);
void user_mutex_wait_on(struct user_mutex *um);

void init_user_mutexes(void);

#endif /* _KERNEL_PROC_USER_MUTEX_H */
