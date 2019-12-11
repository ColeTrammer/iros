#ifndef _KERNEL_PROC_BLOCKERS_H
#define _KERNEL_PROC_BLOCKERS_H 1

#include <bits/time_t.h>
#include <stdbool.h>

struct task;

enum block_type { SLEEP_MILLISECONDS };

struct block_info {
    enum block_type type;
    bool (*should_unblock)(struct block_info *info);
    union {
        struct {
            time_t end_time;
        } sleep_milliseconds_info;
    } __info;
#define sleep_milliseconds_info __info.sleep_milliseconds_info
};

void proc_block_sleep_milliseconds(struct task *current, time_t end_time);

#endif /* _KERNEL_PROC_BLOCKERS_H */