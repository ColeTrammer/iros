#ifndef _KERNEL_PROC_PROCESS_STATE_H
#define _KERNEL_PROC_PROCESS_STATE_H 1

#include <sys/types.h>
#include <stdbool.h>

#include <kernel/util/spinlock.h>

struct proc_state_message {
#define STATE_EXITED      0
#define STATE_INTERRUPTED 1
#define STATE_STOPPED     2
#define STATE_CONTINUED   3
    int type;
    int data;

    struct proc_state_message *next;
};

struct proc_state_message_queue {
    spinlock_t lock;
    struct proc_state_message *start;
    struct proc_state_message *end;
    pid_t pid;
};

struct proc_state_message *proc_create_message(int type, int data);
void proc_add_message(pid_t pid, struct proc_state_message *m);
bool proc_consume_message(pid_t pid, struct proc_state_message *m);

void init_proc_state();

#endif /* _KERNEL_PROC_PROCESS_STATE_H */