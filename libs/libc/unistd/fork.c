#include <sys/syscall.h>
#include <unistd.h>

void (**__prepare_handlers)(void);
int __prepare_handlers_installed;
int __prepare_handlers_max;

void (**__parent_handlers)(void);
int __parent_handlers_installed;
int __parent_handlers_max;

void (**__child_handlers)(void);
int __child_handlers_installed;
int __child_handlers_max;

static void do_prepare_handlers() {
    for (int i = __prepare_handlers_installed - 1; i >= 0; i--) {
        __prepare_handlers[i]();
    }
}

static void do_parent_handlers() {
    for (int i = 0; i < __parent_handlers_installed; i++) {
        __parent_handlers[i]();
    }
}

static void do_child_handlers() {
    for (int i = 0; i < __child_handlers_installed; i++) {
        __child_handlers[i]();
    }
}

pid_t fork() {
    do_prepare_handlers();

    pid_t ret = (pid_t) syscall(SYS_fork);
    if (ret < 0) {
        return ret;
    }

    if (ret == 0) {
        do_child_handlers();
    } else {
        do_parent_handlers();
    }

    return ret;
}
