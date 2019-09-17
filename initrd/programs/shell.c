#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdint.h>
#include <assert.h>

char *read_line() {
    int sz = 300;
    int pos = 0;
    char *buffer = malloc(sz);

    for (;;) {
        assert(pos < sz);

        int c = getchar();

        if (c == EOF || c == '\n') {
            buffer[pos] = '\n';
            buffer[pos + 1] = '\0';
            return buffer;
        }

        buffer[pos++] = c;
    }
}

char **split_line(char *line) {
    int sz = 1024;
    int pos = 0;
    char **tokens = malloc(sz * sizeof(char*));
    char *token;

    token = strtok(line, " \t\r\n\a");

    while (token != NULL) {
        assert(pos < sz);

        tokens[pos++] = token;
        token = strtok(NULL, " \t\r\n\a");
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

struct builtin_op {
    char name[16];
    int (*op)(char **args);
};

#define NUM_BUILTINS 2

static struct builtin_op builtin_ops[NUM_BUILTINS] = {
    { "exit", op_exit },
    { "cd", op_cd }
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

int main() {
    for (;;) {
        char *cwd = __getcwd();
        printf("%s$ ", cwd);
        free(cwd);
        fflush(stdout);

        char *line = read_line();
        
        /* Check If The Line Was Empty */
        if (line == NULL || line[0] == '\n') {
            continue;
        }

        char **args = split_line(line);
        int status = run_program(args);

        free(line);
        free(args);

        if (status == SHELL_EXIT) {
            break;
        }
    }

    return EXIT_SUCCESS;
}