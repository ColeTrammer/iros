#include <errno.h>
#include <fcntl.h>
#include <spawn.h>
#include <unistd.h>

#define SPAWN_FAILURE_EXIT_STATUS 127

typedef int (*exec_function_t)(const char *path, char *const argv[], char *const envp[]);

int __posix_spawn_internal(pid_t *__restrict pidp, const char *__restrict path, const posix_spawn_file_actions_t *fileacts,
                           const posix_spawnattr_t *__restrict attr, char *const args[], char *const envp[], int use_execvpe) {
    exec_function_t do_exec = use_execvpe ? execvpe : execve;

    pid_t pid = fork();
    if (pid < 0) {
        return errno;
    }

    if (pid == 0) {
        unsigned short flags = attr ? attr->__flags : 0;

        if (flags & POSIX_SPAWN_SETPGROUP) {
            if (setpgid(0, attr->__process_group) < 0) {
                _exit(SPAWN_FAILURE_EXIT_STATUS);
            }
        }

        if (flags & POSIX_SPAWN_SETSCHEDULER) {
            // TODO: add and call the sched_setscheduler() function
        } else if (flags & POSIX_SPAWN_SETSCHEDPARAM) {
            // TODO: add and call the sched_setparam() function
        }

        if (flags & POSIX_SPAWN_RESETIDS) {
            // NOTE: these operations will never fail, so no error checks are needed.
            seteuid(getuid());
            setegid(getgid());
        }

        if (flags & POSIX_SPAWN_SETSIGMASK) {
            if (sigprocmask(SIG_SETMASK, &attr->__signal_mask, NULL)) {
                _exit(SPAWN_FAILURE_EXIT_STATUS);
            }
        }

        if (flags & POSIX_SPAWN_SETSIGDEF) {
            struct sigaction act;
            act.sa_flags = 0;
            act.sa_handler = SIG_DFL;

            for (int signal_number = 1; signal_number < _NSIG; signal_number++) {
                if (sigismember(&attr->__signal_default, signal_number)) {
                    if (sigaction(signal_number, &act, NULL) < 0) {
                        _exit(SPAWN_FAILURE_EXIT_STATUS);
                    }
                }
            }
        }

        if (fileacts) {
            for (size_t i = 0; i < fileacts->__action_count; i++) {
                struct __spawn_file_action *action = &fileacts->__action_vector[i];
                switch (action->__type) {
                    case __SPAWN_FILE_ACTION_CLOSE:
                        if (close(action->__fd0) < 0) {
                            _exit(SPAWN_FAILURE_EXIT_STATUS);
                        }
                        break;
                    case __SPAWN_FILE_ACTION_DUP2:
                        if (dup2(action->__fd0, action->__fd1) < 0) {
                            _exit(SPAWN_FAILURE_EXIT_STATUS);
                        }
                        break;
                    case __SPAWN_FILE_ACTION_OPEN:
                        if (close(action->__fd0) < 0) {
                            _exit(SPAWN_FAILURE_EXIT_STATUS);
                        }
                        int new_fd = open(action->__path, action->__oflags, action->__mode);
                        if (new_fd < 0) {
                            _exit(SPAWN_FAILURE_EXIT_STATUS);
                        }
                        if (new_fd != action->__fd0) {
                            if (dup2(new_fd, action->__fd0) < 0) {
                                _exit(SPAWN_FAILURE_EXIT_STATUS);
                            }
                            if (close(new_fd) < 0) {
                                _exit(SPAWN_FAILURE_EXIT_STATUS);
                            }
                        }
                        break;
                }
            }
        }

        do_exec(path, args, envp);
        _exit(SPAWN_FAILURE_EXIT_STATUS);
    }

    if (pidp) {
        *pidp = pid;
    }
    return 0;
}
