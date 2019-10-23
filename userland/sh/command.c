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

static int do_simple_command(struct command_simple *command) {
    pid_t save_pgid = getpid();
    char **args = command->args;

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

        if (command->redirection_info._stdout != NULL) {
            int fd = open(command->redirection_info._stdout, O_CREAT | O_WRONLY | O_TRUNC, 0644);
            if (fd == -1) {
                goto abort_command;
            }
            if (dup2(fd, STDOUT_FILENO) == -1) {
                goto abort_command;
            }
        }

        if (command->redirection_info._stdin != NULL) {
            int fd = open(command->redirection_info._stdin, O_RDONLY);
            if (fd == -1) {
                goto abort_command;
            }
            if (dup2(fd, STDIN_FILENO) == -1) {
                goto abort_command;
            }
        }

        if (command->builtin_op != NULL) {
            exit(builtin_do_op(op, args));
        }
        execvp(args[0], args);

    abort_command:
        perror("Shell");
        exit(EXIT_FAILURE);
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
    int ret = SHELL_CONTINUE;
    for (size_t i = 0; i < pipeline->num_commands; i++) {
        struct command_simple simple_command = pipeline->commands[i];
        ret = do_simple_command(&simple_command);
        if (ret != SHELL_CONTINUE) {
            break;
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
    free(simple_command->args);
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