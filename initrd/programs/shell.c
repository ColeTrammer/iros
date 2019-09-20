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

char **split_line(char *line) {
    int sz = 1024;
    int pos = 0;
    char **tokens = malloc(sz * sizeof(char*));

    bool in_quotes = false;
    char *token_start = line;
    size_t i = 0;
    while (line[i] != '\0') {
        if (!in_quotes && isspace(line[i])) {
            goto add_token;
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

    tokens[pos] = NULL;
    return tokens;
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

int run_program(char **args) {
    for (size_t i = 0; i < NUM_BUILTINS; i++) {
        if (strcmp(args[0], builtin_ops[i].name) == 0) {
            return builtin_ops[i].op(args);
        }
    }

    pid_t pid = fork();
    if (pid == 0) {
        if (execvp(args[0], args) == -1) {
            perror("Shell");
        }
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
        if (input == stdin) {
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

        char **args = split_line(line);

        if (args[0] == NULL) {
            free(line);
            free(args);
            continue;
        }

        int status = run_program(args);

        free(line);
        free(args);

        if (status == SHELL_EXIT) {
            break;
        }
    }

    if (input != stdin) {
        fclose(input);
    }

    return EXIT_SUCCESS;
}