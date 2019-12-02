#ifndef _KERNEL_PROC_PROCESS_STATE_H
#define _KERNEL_PROC_PROCESS_STATE_H 1

#include <stdbool.h>
#include <sys/times.h>
#include <sys/types.h>

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
    pid_t pgid;
    pid_t ppid;

    struct tms times;
    struct proc_state_message_queue *pg_next;
    struct proc_state_message_queue *parent_next;
};

struct proc_state_message *proc_create_message(int type, int data);
void proc_add_message(pid_t pid, struct proc_state_message *m);
pid_t proc_consume_message(pid_t pid, struct proc_state_message *m);
pid_t proc_consume_message_by_pg(pid_t pgid, struct proc_state_message *m);
pid_t proc_consume_message_by_parent(pid_t ppid, struct proc_state_message *m);

void proc_update_pgid(pid_t pid, pid_t pgid);
pid_t proc_get_pgid(pid_t pid);

void init_proc_state();

#endif /* _KERNEL_PROC_PROCESS_STATE_H */