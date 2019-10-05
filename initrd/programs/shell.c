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

struct command {
    char **args;
    char *_stdin;
    char *_stdout;
    char *_stderr;
};

char *read_line(FILE *input) {
    int sz = 1024;
    int pos = 0;
    char *buffer = malloc(sz);

    for (;;) {
        int c = fgetc(input);

        /* In a comment */
        if (c == '#' && (pos == 0 || isspace(buffer[pos - 1]))) {
            c = getc(input);
            while (c != EOF && c != '\n') {
                c = fgetc(input);
            }

            buffer[pos] = '\n';
            buffer[pos + 1] = '\0';
            return buffer;
        }

        if (c == EOF && pos == 0) {
            return NULL;
        }

        if (c == EOF || c == '\n') {
            buffer[pos] = '\n';
            buffer[pos + 1] = '\0';
            return buffer;
        }

        buffer[pos++] = c;

        if (pos + 1 >= sz) {
            sz *= 2;
            buffer = realloc(buffer, sz);
        }
    }
}

void free_command(struct command *command) {
    free(command->args);
    free(command);
}

struct command *split_line(char *line) {
    struct command *command = malloc(sizeof(struct command));
    command->_stderr = NULL;
    command->_stdout = NULL;
    command->_stdin = NULL;

    int sz = 1024;
    int pos = 0;
    char **tokens = malloc(sz * sizeof(char*));

    bool in_quotes = false;
    char *token_start = line;
    size_t i = 0;
    while (line[i] != '\0') {
        if (!in_quotes && (isspace(line[i]))) {
            goto add_token;
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
    return command;
}

#define SHELL_EXIT 1
#define SHELL_CONTINUE 0

static int op_exit(char **args) {
    if (args[1] != NULL) {
        printf("Usage: %s\n", args[0]);
        return SHELL_CONTINUE;
    }

    /* Exit */
    return SHELL_EXIT;
}

static int op_cd(char **args) {
    if (!args[1] || args[2]) {
        printf("Usage: %s <dir>\n", args[0]);
        return SHELL_CONTINUE;
    }

    int ret = chdir(args[1]);
    if (ret != 0) {
        perror("Shell");
    }

    return SHELL_CONTINUE;
}

static int op_echo(char **args) {
    if (!args[1] || args[2]) {
        printf("Usage: %s <string>\n", args[0]);
        return SHELL_CONTINUE;
    }

    puts(args[1]);
    return SHELL_CONTINUE;
}

struct builtin_op {
    char name[16];
    int (*op)(char **args);
};

#define NUM_BUILTINS 3

static struct builtin_op builtin_ops[NUM_BUILTINS] = {
    { "exit", op_exit },
    { "cd", op_cd },
    { "echo", op_echo }
};

int run_program(struct command *command) {
    char **args = command->args;
    for (size_t i = 0; i < NUM_BUILTINS; i++) {
        if (strcmp(args[0], builtin_ops[i].name) == 0) {
            return builtin_ops[i].op(args);
        }
    }

    pid_t pid = fork();
    if (pid == 0) {
        if (command->_stdout != NULL) {
            int fd = open(command->_stdout, O_CREAT | O_WRONLY | O_TRUNC, 0644);
            if (fd == -1) {
                goto abort_command;
            }
            if (dup2(fd, STDOUT_FILENO) == -1) {
                goto abort_command;
            }

            close(fd);
        }

        if (command->_stdin != NULL) {
            int fd = open(command->_stdin, O_RDONLY);
            if (fd == -1) {
                goto abort_command;
            }
            if (dup2(fd, STDIN_FILENO) == -1) {
                goto abort_command;
            }

            close(fd);
        }

        execvp(args[0], args);

    abort_command:
        perror("Shell");
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("Shell");
    } else {
        int status;
        do {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status));
    }

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

int main(int argc, char **argv) {
    FILE *input = stdin;
    if (argc == 2) {
        input = fopen(argv[1], "r");
        if (input == NULL) {
            perror("Shell");
            return EXIT_FAILURE;
        }
    } else if (argc > 2) {
        printf("Usage: %s [script]\n", argv[0]);
        return EXIT_SUCCESS;
    }

    for (;;) {
        if (input == stdin && isatty(STDIN_FILENO) && isatty(STDOUT_FILENO)) {
            char *cwd = __getcwd();
            printf("\033[32m%s\033[37m:\033[36m%s\033[37m$ ", "root@os_2", cwd);
            free(cwd);
        }
        fflush(stdout);

        char *line = read_line(input);

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

        struct command *command = split_line(line);

        if (command->args[0] == NULL) {
            free(line);
            free_command(command);
            continue;
        }

        int status = run_program(command);

        free(line);
        free_command(command);

        if (status == SHELL_EXIT) {
            break;
        }
    }

    if (input != stdin) {
        fclose(input);
    }

    return EXIT_SUCCESS;
}