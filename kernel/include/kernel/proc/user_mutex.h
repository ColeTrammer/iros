#ifndef _KERNEL_PROC_USER_MUTEX_H
#define _KERNEL_PROC_USER_MUTEX_H 1

#include <stdint.h>

#include <kernel/proc/task.h>
#include <kernel/util/spinlock.h>

struct user_mutex {
    struct user_mutex *next;
    uintptr_t phys_addr;
    spinlock_t lock;
    struct task *next_to_wake_up;
};

struct user_mutex *get_user_mutex_locked(int *addr);
struct user_mutex *get_user_mutex_locked_with_waiters_or_else_write_value(int *addr, int value);
void unlock_user_mutex(struct user_mutex *um);
void add_to_user_mutex_queue(struct user_mutex *m, struct task *task);
void wake_first_user_mutex(struct user_mutex *m);

void init_user_mutexes();

#endif /* _KERNEL_PROC_USER_MUTEX_H */