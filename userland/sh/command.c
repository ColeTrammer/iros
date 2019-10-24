#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include <sys/wait.h>

#include "builtin.h"
#include "command.h"

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

static void init_simple_command(struct command_simple *simple_command) {
    memset(simple_command, 0, sizeof(struct command_simple));
}

static void init_pipeline(struct command_pipeline *pipeline, size_t num) {
    pipeline->commands = calloc(num, sizeof(struct command_simple));
    pipeline->num_commands = num;
}

struct command *command_construct(enum command_type type, ...) {
    va_list args;
    va_start(args, type);

    struct command *command = malloc(sizeof(struct command));
    command->type = type;
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
        case COMMAND_LIST:
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
            if (close(desc->desc.fd) == -1) {
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

static int do_simple_command(struct command_simple *command) {
    pid_t save_pgid = getpid();
    char **args = command->we.we_wordv;

    struct builtin_op *op = builtin_find_op(args[0]);
    if (builtin_should_run_immediately(op)) {
        return builtin_do_op(op, args);
    } else if (op) {
        command->builtin_op = op;
    }

    pid_t pid = fork();

    // Child
    if (pid == 0) {
        if (isatty(STDOUT_FILENO)) {
            setpgid(0, 0);
            tcsetpgrp(STDOUT_FILENO, getpid());
            struct sigaction to_set;
            to_set.sa_handler = SIG_DFL;
            to_set.sa_flags = 0;
            sigaction(SIGINT, &to_set, NULL); 

            sigset_t mask_restore;
            sigemptyset(&mask_restore);
            sigprocmask(SIG_SETMASK, &mask_restore, NULL);
        }

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
    }

    // Parent
    if (isatty(STDOUT_FILENO)) {
        setpgid(pid, pid);
        tcsetpgrp(STDOUT_FILENO, pid);
    }

    int status;
    do {
        waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSTOPPED(status) && !WIFSIGNALED(status));

    if (isatty(STDOUT_FILENO)) {
        tcsetpgrp(STDOUT_FILENO, save_pgid);
    }

    return SHELL_CONTINUE;
}

static int do_pipeline(struct command_pipeline *pipeline) {
    int fds[(pipeline->num_commands - 1) * 2];
    for (size_t i = 0; i < pipeline->num_commands - 1; i++) {
        if (pipe(fds + (i * 2))) {
            perror("sh");
            return SHELL_CONTINUE;
        }
    }

    int ret = SHELL_CONTINUE;
    for (size_t i = 0; i < pipeline->num_commands; i++) {
        struct command_simple simple_command = pipeline->commands[i];

        if (i != pipeline->num_commands - 1) {
            init_redirection(&simple_command.redirection_info, STDOUT_FILENO, REDIRECT_FD, fds[i * 2 + 1]);
        }

        if (i != 0) {
            init_redirection(&simple_command.redirection_info, STDIN_FILENO, REDIRECT_FD, fds[(i - 1) * 2]);
        }

        ret = do_simple_command(&simple_command);
        if (ret != SHELL_CONTINUE) {
            break;
        }

        if (i != pipeline->num_commands - 1) {
            close(fds[i * 2 + 1]);
        }

        if (i != 0) {
            close(fds[(i - 1) * 2]);
        }
    }

    return ret;
}

int command_run(struct command *command) {
    switch (command->type) {
        case COMMAND_SIMPLE:
            return do_simple_command(&command->command.simple_command);
        case COMMAND_PIPELINE:
            return do_pipeline(&command->command.pipeline);
        case COMMAND_LIST:
        case COMMAND_COMPOUND:
        case COMMAND_FUNCTION_DECLARATION:
        default:
            assert(false);
            return SHELL_EXIT;
    }
}

static void cleanup_simple_command(struct command_simple *simple_command) {
    wordfree(&simple_command->we);
}

void command_cleanup(struct command *command) {
    switch (command->type) {
        case COMMAND_SIMPLE: {
            cleanup_simple_command(&command->command.simple_command);
            break;
        }
        case COMMAND_PIPELINE: {
            struct command_pipeline pipeline = command->command.pipeline;
            for (size_t i = 0; i < pipeline.num_commands; i++) {
                cleanup_simple_command(&pipeline.commands[i]);
            }
            free(pipeline.commands);
            break;
        }
        case COMMAND_LIST:
        case COMMAND_COMPOUND:
        case COMMAND_FUNCTION_DECLARATION:
        default:
            assert(false);
            break;
    }

    free(command);
}