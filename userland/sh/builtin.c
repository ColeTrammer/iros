#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#include "builtin.h"
#include "job.h"

static int op_exit(char **args) {
    if (args[1] != NULL) {
        printf("Usage: %s\n", args[0]);
        return 0;
    }

    exit(0);
}

static int op_cd(char **args) {
    if (!args[1] || args[2]) {
        printf("Usage: %s <dir>\n", args[0]);
        return 0;
    }

    int ret = chdir(args[1]);
    if (ret != 0) {
        perror("Shell");
    }

    return 0;
}

static int op_echo(char **args) {
    if (!args[1]) {
        printf("%c", '\n');
        return 0;
    }

    size_t i = 1;
    for (;;) {
        printf("%s", args[i]);
        if (args[i + 1] != NULL) {
            printf("%c", ' ');
            i++;
        } else {
            break;
        }
    }

    printf("%c", '\n');
    return 0;
}

static int op_export(char **argv) {
    if (!argv[1]) {
        printf("Usage: %s <key=value>\n", argv[0]);
        return 0;
    }

    for (size_t i = 1; argv[i] != NULL; i++) {
        char *equals = strchr(argv[i], '=');
        if (equals == NULL) {
            fprintf(stderr, "Invalid environment string: %s\n", argv[i]);
            continue;
        }
        *equals = '\0';

        if (setenv(argv[i], equals + 1, 1)) {
            perror("shell");
            return 0;
        }
    }

    return 0;
}

static int op_unset(char **argv) {
    if (!argv[1]) {
        printf("Usage: %s <key>\n", argv[0]);
        return 0;
    }

    for (size_t i = 1; argv[i] != NULL; i++) {
        if (unsetenv(argv[i])) {
            perror("shell");
            return 0;
        }
    }

    return 0;
}

static int op_jobs(char **argv) {
    (void) argv;

    job_print_all();
    return 0;
}

static int op_fg(char **argv) {
    if (!argv[1] || argv[2]) {
        printf("Usage: %s <job>", argv[0]);
    }

    struct job_id id;
    if (argv[1][0] == '%') {
        id = job_id(JOB_ID, atoi(argv[1] + 1));
    } else {
        id = job_id(JOB_PGID, atoi(argv[1]));
    }

    return job_run(id);
}

static struct builtin_op builtin_ops[NUM_BUILTINS] = {
    { "exit", op_exit, true },
    { "cd", op_cd, true },
    { "echo", op_echo, false },
    { "export", op_export, true },
    { "unset", op_unset, true },
    { "jobs", op_jobs, true },
    { "fg", op_fg, true }
};

struct builtin_op *builtin_find_op(char *name) {
    for (size_t i = 0; i < NUM_BUILTINS; i++) {
        if (strcmp(builtin_ops[i].name, name) == 0) {
            return &builtin_ops[i];
        }
    }

    return NULL;
}

bool builtin_should_run_immediately(struct builtin_op *op) {
    return op && op->run_immediately;
}

int builtin_do_op(struct builtin_op *op, char **args) {
    assert(op);
    return op->op(args);
}