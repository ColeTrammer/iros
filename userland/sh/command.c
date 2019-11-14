#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "builtin.h"
#include "command.h"
#include "job.h"

#ifndef USERLAND_NATIVE

static word_special_t special_vars = { {
    "", "", "0", NULL, "", NULL, NULL, "/bin/sh"
} };

static void __set_exit_status(int n) {
    free(special_vars.vals[WRDE_SPECIAL_QUEST]);
    special_vars.vals[WRDE_SPECIAL_QUEST] = malloc(4);
    sprintf(special_vars.vals[WRDE_SPECIAL_QUEST], "%d", n);
}

#else

static int last_exit_status;

static void __set_exit_status(int n) {
    last_exit_status = n;
}

#endif /* USERLAND_NATIVE */

void set_exit_status(int n) {
    assert(WIFEXITED(n) || WIFSIGNALED(n) || WIFSTOPPED(n));

    __set_exit_status(WIFEXITED(n) ? WEXITSTATUS(n) : 
                      WIFSTOPPED(n) ? 0 : (127 + WTERMSIG(n)));
}

#ifndef USERLAND_NATIVE

int get_last_exit_status() {
    return atoi(special_vars.vals[WRDE_SPECIAL_QUEST]);
}

#else

int get_last_exit_status() {
    return last_exit_status;
}

#endif /* USERLAND_NATIVE */

// FIXME: redirection actually needs to be a queue, not an array, since the order of specified redirections
//        should cause different behavior, and currently the order is based on the target fd not the order
//        the redirection command is inputted.
void init_redirection(struct redirection_info *info,  int target_fd, enum redirection_method method, ...) {
    va_list args;
    va_start(args, method);

    struct redirection_desc *desc = &info->redirection_descs[target_fd];

    desc->target_fd = target_fd;
    desc->method = method;
    switch (method) {
        case REDIRECT_APPEND_FILE:
        case REDIRECT_FILE:
            desc->desc.file = va_arg(args, char*);
            break;
        case REDIRECT_FD:
            desc->desc.fd = va_arg(args, int);
            break;
        case REDIRECT_NONE:
        default:
            break;
    }

    va_end(args);
}

void init_simple_command(struct command_simple *simple_command) {
    memset(simple_command, 0, sizeof(struct command_simple));
#ifndef USERLAND_NATIVE
    simple_command->we.we_special_vars = &special_vars;
#endif /* USERLAND_NATIVE */
}

void init_pipeline(struct command_pipeline *pipeline, size_t num) {
    pipeline->commands = calloc(num, sizeof(struct command_simple));
    pipeline->num_commands = num;
}

void init_list(struct command_list *list, enum command_list_connector *connectors, size_t len) {
    list->commands = calloc(len, sizeof(struct command_pipeline));
    list->connectors = connectors;
    list->num_commands = len;
}

struct command *command_construct(enum command_type type, enum command_mode mode, ...) {
    va_list args;
    va_start(args, mode);

    struct command *command = malloc(sizeof(struct command));
    command->type = type;
    command->mode = mode;
    switch (type) {
        case COMMAND_SIMPLE: {
            init_simple_command(&command->command.simple_command);
            break;
        }
        case COMMAND_PIPELINE: {
            size_t num = va_arg(args, size_t);
            init_pipeline(&command->command.pipeline, num);
            break;
        }
        case COMMAND_LIST: {
            enum command_list_connector *connectors = va_arg(args, enum command_list_connector*);
            size_t len = va_arg(args, size_t);
            init_list(&command->command.list, connectors, len);
            break;
        }
        case COMMAND_COMPOUND:
        case COMMAND_FUNCTION_DECLARATION:
        default:
            assert(false);
            break;
    }

    va_end(args);
    return command;
}

static bool handle_redirection(struct redirection_desc *desc) {
    int flags = 0;
    switch (desc->method) {
        case REDIRECT_APPEND_FILE: {
            flags |= O_APPEND;
        }
        // Fall through
        case REDIRECT_FILE: {
            flags |= O_CREAT | O_WRONLY | O_TRUNC;
            int fd = open(desc->desc.file, flags, 0644);
            if (fd == -1) {
                return false;
            }
            if (dup2(fd, desc->target_fd) == -1) {
                return false;
            }
            if (close(fd)) {
                return false;
            }
            break;
        }
        case REDIRECT_FD: {
            if (dup2(desc->desc.fd, desc->target_fd) == -1) {
                return false;
            }
            break;
        }
        case REDIRECT_NONE:
        default:
            break;
    }

    return true;
}

// Does the command and returns the pid of the command for the caller to wait on, (returns -1 on error) (exit status if bulit in command)
static pid_t __do_simple_command(struct command_simple *command, enum command_mode mode, bool *was_builtin, pid_t to_set_pgid) {
    char **args = command->we.we_wordv;

    struct builtin_op *op = builtin_find_op(args[0]);
    if (builtin_should_run_immediately(op)) {
        *was_builtin = true;
        return builtin_do_op(op, args);
    } else if (op) {
        command->builtin_op = op;
    }

    pid_t pid = fork();

    // Child
    if (pid == 0) {
        setpgid(to_set_pgid, to_set_pgid);
        if (isatty(STDOUT_FILENO) && mode == COMMAND_FOREGROUND) {
            tcsetpgrp(STDOUT_FILENO, to_set_pgid == 0 ? getpid() : to_set_pgid);
        }

        struct sigaction to_set;
        to_set.sa_handler = SIG_DFL;
        to_set.sa_flags = 0;
        sigaction(SIGINT, &to_set, NULL); 

        sigset_t mask_restore;
        sigemptyset(&mask_restore);
        sigprocmask(SIG_SETMASK, &mask_restore, NULL);

        for (size_t i = 0; i < MAX_REDIRECTIONS; i++) {
            if (!handle_redirection(&command->redirection_info.redirection_descs[i])) {
                goto abort_command;
            }
        }

        if (command->builtin_op != NULL) {
            exit(builtin_do_op(op, args));
        }
        execvp(args[0], args);

    abort_command:
        perror("Shell");
        _exit(127);
    } else if (pid < 0) {
        perror("Shell");
        return -1;
    }

    return pid;
}

static int do_pipeline(struct command_pipeline *pipeline, enum command_mode mode) {
    int fds[(pipeline->num_commands - 1) * 2];
    for (size_t i = 0; i < pipeline->num_commands - 1; i++) {
        if (pipe(fds + (i * 2))) {
            perror("sh");
            return 0;
        }
    }

    pid_t save_pgid = getpid();
    pid_t pgid = 0;
    pid_t last = 0;

    size_t num_to_wait_on = 0;
    pid_t pids[JOB_MAX_PIDS];

    size_t i;
    for (i = 0; i < pipeline->num_commands; i++) {
        struct command_simple simple_command = pipeline->commands[i];

        if (i != pipeline->num_commands - 1) {
            init_redirection(&simple_command.redirection_info, STDOUT_FILENO, REDIRECT_FD, fds[i * 2 + 1]);
        }

        if (i != 0) {
            init_redirection(&simple_command.redirection_info, STDIN_FILENO, REDIRECT_FD, fds[(i - 1) * 2]);
        }

        bool is_builtin = false;
        pid_t pid = __do_simple_command(&simple_command, mode, &is_builtin, pgid);

        if (i != pipeline->num_commands - 1) {
            close(fds[i * 2 + 1]);
        }

        if (i != 0) {
            close(fds[(i - 1) * 2]);
        }

        if (is_builtin) {
            __set_exit_status(pid);
            continue;
        }

        if (pid == -1) {
            break;
        }

        if (pgid == 0) {
            pgid = pid;
            if (isatty(STDOUT_FILENO) && mode == COMMAND_FOREGROUND) {
                tcsetpgrp(STDOUT_FILENO, pgid);
            }
        }

        setpgid(pid, pgid);

        pids[num_to_wait_on++] = pid;
        last = pid;
    }

    for (size_t j = (i - 1) * 2; j < (pipeline->num_commands - 1) * 2; j += 2) {
        if (close(fds[j]) ||
            close(fds[j + 1])) {
            return -1;
        }
    }


    if (mode == COMMAND_FOREGROUND) {
        if (num_to_wait_on > 0) {
            int wstatus;
            for (size_t num_waited = 0; num_waited < num_to_wait_on; num_waited++) {
                int ret;
                do {
                    ret = waitpid(-pgid, &wstatus, WUNTRACED);
                } while (ret != -1 && !WIFEXITED(wstatus) && !WIFSIGNALED(wstatus) &&!WIFSTOPPED(wstatus));

                if (ret == -1) {
                    return -1;
                }

                if (WIFSTOPPED(wstatus)) {
                    killpg(pgid, SIGSTOP);
                    job_add(pgid, pids, num_to_wait_on - num_waited, STOPPED);
                    printf("%c", '\n');
                    print_job(get_jid_from_pgid(pgid));
                    break; // Not sure what to set the exit status to...
                }

                if (WIFSIGNALED(wstatus) && num_waited == num_to_wait_on - 1) {
                    if (isatty(STDERR_FILENO)) {
                        fprintf(stderr, "%c", '\n');
                    }
                }

                if (ret == last) {
                    set_exit_status(wstatus);
                }
            }
        }

        if (isatty(STDOUT_FILENO)) {
            tcsetpgrp(STDOUT_FILENO, save_pgid);
        }
    } else {
        job_add(pgid, pids, i, RUNNING);
        print_job(get_jid_from_pgid(pgid));
    }

    return i == pipeline->num_commands ? 0 : -1;
}

static int do_command_list(struct command_list *list, enum command_mode mode) {
    assert(mode != COMMAND_BACKGROUND || list->num_commands <= 1);
    for (size_t i = 0; i < list->num_commands; i++) {
        int ret = do_pipeline(&list->commands[i], mode);
        if (ret != 0) {
            return ret;
        }

        int status = get_last_exit_status();
        if ((list->connectors[i] == COMMAND_AND && status != 0) || (list->connectors[i] == COMMAND_OR && status == 0)) {
            // Advance until next sequential command
            while (i < list->num_commands - 1 && list->connectors[i++] != COMMAND_SEQUENTIAL);
        }
    }

    return 0;
}

int command_run(struct command *command) {
    switch (command->type) {
        case COMMAND_SIMPLE:
            // Should be parsed into list of length 1
            assert(false);
            return -1;
        case COMMAND_PIPELINE:
            // Should be parsed into a list of length 1
            assert(false);
            return -1;
        case COMMAND_LIST:
            return do_command_list(&command->command.list, command->mode);
        case COMMAND_COMPOUND:
        case COMMAND_FUNCTION_DECLARATION:
        default:
            assert(false);
            return -1;
    }
}

static void cleanup_simple_command(struct command_simple *simple_command) {
    wordfree(&simple_command->we);
}

static void cleanup_pipeline(struct command_pipeline *pipeline) {
    for (size_t i = 0; i < pipeline->num_commands; i++) {
        cleanup_simple_command(&pipeline->commands[i]);
    }
    free(pipeline->commands);
}

void command_cleanup(struct command *command) {
    switch (command->type) {
        case COMMAND_SIMPLE: {
            cleanup_simple_command(&command->command.simple_command);
            break;
        }
        case COMMAND_PIPELINE: {
            cleanup_pipeline(&command->command.pipeline);
            break;
        }
        case COMMAND_LIST: {
            struct command_list list = command->command.list;
            for (size_t i = 0; i < list.num_commands; i++) {
                cleanup_pipeline(&list.commands[i]);
            }
            free(list.commands);
            free(list.connectors);
            break;
        }
        case COMMAND_COMPOUND:
        case COMMAND_FUNCTION_DECLARATION:
        default:
            assert(false);
            break;
    }

    free(command);
}

void command_init_special_vars() {
#ifndef USERLAND_NATIVE
    special_vars.vals[WRDE_SPECIAL_QUEST] = strdup("0");
    special_vars.vals[WRDE_SPECIAL_DOLLAR] = malloc(10);
    sprintf(special_vars.vals[WRDE_SPECIAL_DOLLAR], "%d", getpid());
    special_vars.vals[WRDE_SPECIAL_EXCLAM] = strdup("");
#endif /* USERLAND_NATIVE */
}
