#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdint.h>
#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <setjmp.h>

#include "builtin.h"
#include "input.h"

struct command {
    char **args;
    char *_stdin;
    char *_stdout;
    char *_stderr;
    struct builtin_op *builtin_op;
};

void free_commands(struct command **commands) {
    size_t i = 0;
    struct command *command = commands[i++];
    while (command != NULL) {
        free(command->args);
        free(command);

        command = commands[i++];
    }

    free(commands);
}

size_t get_num_commands(struct command **commands) {
    size_t i = 0;
    struct command *command = commands[i++];
    while (command != NULL) {
        command = commands[i++];
    }

    return i - 1;
}

struct command **split_line(char *line) {
    size_t max_commands = 10;
    struct command **commands = calloc(max_commands, sizeof(struct command*));
    struct command *command;

    bool in_quotes = false;
    char *token_start = line;
    size_t i = 0;
    size_t j = 0;
    while (line[i] != '\0') {
        int sz = 1024;
        int pos = 0;
        char **tokens = malloc(sz * sizeof(char*));

        command = malloc(sizeof(struct command));
        command->_stderr = NULL;
        command->_stdout = NULL;
        command->_stdin = NULL;
        command->builtin_op = NULL;

        if (j >= max_commands - 1) {
            max_commands *= 2;
            commands = realloc(commands, max_commands * sizeof(struct command*));
        }

        commands[j++] = command;

        while (line[i] != '\0') {
            if (!in_quotes && (isspace(line[i]))) {
                goto add_token;
            }

            /* Handle pipes */
            else if (!in_quotes && line[i] == '|') {
                while (isspace(line[++i]));
                token_start = line + i;
                break;
            }

            /* Handle output redirection */
            else if (!in_quotes && line[i] == '>') {
                while (isspace(line[++i]));
                command->_stdout = line + i;
                while (!isspace(line[i])) { i++; }
                line[i++] = '\0';
                token_start = line + i;
                continue;
            }

            /* Handles input redirection */
            else if (!in_quotes && line[i] == '<') {
                while (isspace(line[++i]));
                command->_stdin = line + i;
                while (!isspace(line[i])) { i++; }
                line[i++] = '\0';
                token_start = line + i;
                continue;
            }

            /* Assumes quote is at beginning of token */
            else if (!in_quotes && line[i] == '"') {
                in_quotes = true;
                token_start++;
                i++;
                continue;
            }

            else if (in_quotes && line[i] == '"') {
                in_quotes = false;
                goto add_token;
            }

            else {
                i++;
                continue;
            }

        add_token:
            line[i++] = '\0';
            tokens[pos++] = token_start;
            while (isspace(line[i])) { i++; }
            token_start = line + i;

            if (pos + 1 >= sz) {
                sz *= 2;
                tokens = realloc(tokens, sz * sizeof(char*));
            }
        }

        if (in_quotes) {
            pos = 0;
            fprintf(stderr, "Shell: %s\n", "Invalid string format");
        }

        tokens[pos] = NULL;
        command->args = tokens;
    }

    commands[j] = NULL;
    return commands;
}

int run_commands(struct command **commands) {
    size_t num_commands = get_num_commands(commands);

    int *pipes = num_commands > 1 ? calloc(num_commands - 1, 2 * sizeof(int)) : NULL;
    for (size_t j = 0; j < num_commands - 1; j++) {
        if (pipe(pipes + j * 2) == -1) {
            perror("Shell");
            return SHELL_CONTINUE;
        }
    }

    pid_t save_pgid = getpid();
    size_t i = 0;
    struct command *command = commands[i];
    while (command != NULL) {
        char **args = command->args;

        struct builtin_op *op = builtin_find_op(args[0]);
        if (builtin_should_run_immediately(op)) {
            return builtin_do_op(op, args);
        } else if (op) {
            command->builtin_op = op;
        }

        pid_t pid = fork();

        /* Child */
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

            if (command->_stdout != NULL && i == num_commands - 1) {
                int fd = open(command->_stdout, O_CREAT | O_WRONLY | O_TRUNC, 0644);
                if (fd == -1) {
                    goto abort_command;
                }
                if (dup2(fd, STDOUT_FILENO) == -1) {
                    goto abort_command;
                }
            }

            if (command->_stdin != NULL && i == 0) {
                int fd = open(command->_stdin, O_RDONLY);
                if (fd == -1) {
                    goto abort_command;
                }
                if (dup2(fd, STDIN_FILENO) == -1) {
                    goto abort_command;
                }
            }

            /* First program in chain */
            if (num_commands > 1 && i == 0) {
                if (dup2(pipes[i + 1], STDOUT_FILENO) == -1) {
                    goto abort_command;
                }
            }

            /* Last program in chain */
            else if (num_commands > 1 && i == num_commands - 1) {
                if (dup2(pipes[(i - 1) * 2], STDIN_FILENO) == -1) {
                    goto abort_command;
                }
            }

            /* Any other program in chain */
            else if (num_commands > 1) {
                if (dup2(pipes[i * 2 + 1], STDOUT_FILENO) == -1) {
                    goto abort_command;
                }

                if (dup2(pipes[(i - 1) * 2], STDIN_FILENO) == -1) {
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

        /* Parent */
        else {
            if (isatty(STDOUT_FILENO)) {
                setpgid(pid, pid);
                tcsetpgrp(STDOUT_FILENO, pid);
            }

            /* Close write pipe for the last process */
            if (num_commands > 1 && i == 0) {
                close(pipes[i * 2 + 1]);
            }

            else if (num_commands > 1 && i == num_commands - 1) {
                close(pipes[(i - 1) * 2]);
            }

            else if (num_commands > 1) {
                close(pipes[i * 2 + 1]);
                close(pipes[(i - 1) * 2]);
            }

            int status;
            do {
                waitpid(pid, &status, WUNTRACED);
            } while (!WIFEXITED(status) && !WIFSTOPPED(status) && !WIFSIGNALED(status));
        }

        command = commands[++i];
    }

    if (isatty(STDOUT_FILENO)) {
        tcsetpgrp(STDOUT_FILENO, save_pgid);
    }

    free(pipes);
    return SHELL_CONTINUE;
}

static char *__getcwd() {
    size_t size = 50;
    char *buffer = malloc(size);
    char *cwd = getcwd(buffer, size);
    
    while (cwd == NULL) {
        free(buffer);
        size *= 2;
        buffer = malloc(size);
        cwd = getcwd(buffer, size);
    }

    return cwd;
}

static char *line = NULL;
static struct command **commands = NULL;
static sigjmp_buf env;
static volatile sig_atomic_t jump_active = 0;

static void on_int(int signo) {
    assert(signo == SIGINT);
    if (!jump_active) {
        return;
    }

    siglongjmp(env, 1);
}

static void on_child(int signo) {
    assert(signo == SIGCHLD);
}

int main(int argc, char **argv) {
    struct input_source input_source;

    // Respect -c
    if (argc == 3 && strcmp(argv[1], "-c") == 0) {
        input_source.mode = INPUT_STRING;
        input_source.source.string_input_source = input_create_string_input_source(argv[2]);
    } else if (argc == 2) {
        input_source.mode = INPUT_FILE;
        input_source.source.file = fopen(argv[1], "r");
        if (input_source.source.file == NULL) {
            perror("Shell");
            return EXIT_FAILURE;
        }
    } else if (argc > 2) {
        printf("Usage: %s [-c] [script]\n", argv[0]);
        return EXIT_SUCCESS;
    } else {
        input_source.mode = INPUT_TTY;
        input_source.source.tty = stdin;
    }

    if (isatty(STDOUT_FILENO)) {
        struct sigaction to_set;
        to_set.sa_handler = &on_int;
        to_set.sa_flags = 0;
        sigaction(SIGINT, &to_set, NULL);

        to_set.sa_flags = SA_RESTART;
        to_set.sa_handler = &on_child;
        sigaction(SIGCHLD, &to_set, NULL);

        to_set.sa_flags = 0;
        to_set.sa_handler = SIG_IGN;
        sigaction(SIGTTOU, &to_set, NULL);

        sigset_t sigset;
        sigemptyset(&sigset);
        sigaddset(&sigset, SIGTSTP);
        sigprocmask(SIG_SETMASK, &sigset, NULL);
    }

    for (;;) {
        if (sigsetjmp(env, 1) == 1) {
            if (line) { free(line); }
            if (commands) { free_commands(commands); }
            printf("%c", '\n');
        }
        jump_active = 1;

        if (input_source.mode == INPUT_TTY) {
            char *cwd = __getcwd();
            printf("\033[32m%s\033[37m:\033[36m%s\033[37m$ ", "root@os_2", cwd);
            free(cwd);
        }
        fflush(stdout);

        line = input_get_line(&input_source);

        /* Check if we reached EOF */
        if (line == NULL) {
            free(line);
            break;
        }

        /* Check If The Line Was Empty */
        if (line[0] == '\n') {
            free(line);
            continue;
        }

        struct command **commands = split_line(line);

        if (commands == NULL || commands[0] == NULL || commands[0]->args[0] == NULL) {
            free(line);
            free_commands(commands);
            continue;
        }

        int status = run_commands(commands);

        free(line);
        free_commands(commands);

        if (status == SHELL_EXIT) {
            break;
        }
    }

    input_cleanup(&input_source);

    return EXIT_SUCCESS;
}