#include <sys/os_2.h>
#include <sys/syscall.h>

int create_task(struct create_task_args *create_task_args) {
    return syscall(SYS_CREATE_TASK, create_task_args);
}
