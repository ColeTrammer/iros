#include <assert.h>
#include <liim/hash_map.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "builtin.h"
#include "command.h"
#include "input.h"
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

static int op_colon(char **) {
    return 0;
}

static int op_true(char **) {
    return 0;
}

static int op_false(char **) {
    return 1;
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

    job_check_updates(false);
    job_print_all();
    return 0;
}

static int op_fg(char **argv) {
    if (!argv[1] || argv[2]) {
        printf("Usage: %s <job>\n", argv[0]);
        return 0;
    }

    job_check_updates(true);

    struct job_id id;
    if (argv[1][0] == '%') {
        id = job_id(JOB_ID, atoi(argv[1] + 1));
    } else {
        id = job_id(JOB_PGID, atoi(argv[1]));
    }

    return job_run(id);
}

static int op_bg(char **argv) {
    if (!argv[1] || argv[2]) {
        printf("Usage: %s <job>\n", argv[0]);
        return 0;
    }

    job_check_updates(true);

    struct job_id id;
    if (argv[1][0] == '%') {
        id = job_id(JOB_ID, atoi(argv[1] + 1));
    } else {
        id = job_id(JOB_PGID, atoi(argv[1]));
    }

    return job_run_background(id);
}

static int op_kill(char **argv) {
    if (!argv[1] || argv[2]) {
        printf("Usage: %s <job>\n", argv[0]);
        return 0;
    }

    struct job_id id;
    if (argv[1][0] == '%') {
        id = job_id(JOB_ID, atoi(argv[1] + 1));
    } else {
        id = job_id(JOB_PGID, atoi(argv[1]));
    }

    int ret = killpg(get_pgid_from_id(id), SIGTERM);

    job_check_updates(true);
    return ret;
}

static int op_history(char **argv) {
    if (argv[1]) {
        printf("Usage: %s\n", argv[0]);
        return 0;
    }

    print_history();
    return 0;
}

static int op_break(char **argv) {
    int break_count = 1;
    if (argv[1] != NULL) {
        break_count = atoi(argv[1]);
    }

    if (get_loop_depth_count() == 0) {
        fprintf(stderr, "Break is meaningless outside of for,while,until.\n");
        return 0;
    }

    if (break_count == 0) {
        fprintf(stderr, "Invalid loop number.\n");
        return 1;
    }

    set_break_count(break_count);
    return 0;
}

static int op_continue(char **argv) {
    int continue_count = 1;
    if (argv[1] != NULL) {
        continue_count = atoi(argv[1]);
    }

    if (get_loop_depth_count() == 0) {
        fprintf(stderr, "Continue is meaningless outside of for,while,until.\n");
        return 0;
    }

    if (continue_count == 0) {
        fprintf(stderr, "Invalid loop number.\n");
        return 1;
    }

    set_continue_count(continue_count);
    return 0;
}

static int op_dot(char **argv) {
    if (argv[1] == NULL) {
        fprintf(stderr, "Usage: %s <filename> [args]\n", argv[0]);
        return 2;
    }

    struct input_source source;
    source.mode = INPUT_FILE;
    source.source.file = fopen(argv[1], "r");
    if (!source.source.file) {
        return 1;
    }

    int i;
    for (i = 2; argv[i] != NULL; i++)
        ;

    command_push_position_params(PositionArgs(argv + 2, i - 2));
    return do_command_from_source(&source);
}

extern HashMap<String, String> g_aliases;

static String sh_escape(const String &string) {
    // FIXME: should escape the literal `'` with `'\''`
    return String::format("'%s'", string.string());
}

static int op_alias(char **argv) {
    if (argv[1] == nullptr) {
        g_aliases.for_each_key([&](const String &name) {
            printf("alias %s=%s\n", name.string(), sh_escape(*g_aliases.get(name)).string());
        });
        return 0;
    }

    bool any_failed = false;
    for (int i = 1; argv[i] != nullptr; i++) {
        String arg(argv[i]);
        int equal_index = arg.index_of('=');
        if (equal_index == -1) {
            auto *alias_name = g_aliases.get(arg);
            if (!alias_name) {
                any_failed = true;
                continue;
            }

            printf("alias %s=%s\n", arg.string(), sh_escape(*alias_name).string());
            continue;
        }

        arg[equal_index] = '\0';
        String name(arg.string());
        String alias(arg.string() + equal_index + 1);
        g_aliases.put(name, alias);
    }

    return any_failed ? 1 : 0;
}

static int op_unalias(char **argv) {
    if (argv[1] == nullptr) {
        fprintf(stderr, "Usage: %s [-a] name [...]\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "-a") == 0) {
        g_aliases.clear();
        return 0;
    }

    bool any_failed = false;
    for (int i = 1; argv[i] != nullptr; i++) {
        String s(argv[i]);

        if (!g_aliases.get(s)) {
            any_failed = true;
            continue;
        }
        g_aliases.remove(s);
    }

    return any_failed ? 1 : 0;
}

static struct builtin_op builtin_ops[NUM_BUILTINS] = {
    { "exit", op_exit, true },       { "cd", op_cd, true },       { "echo", op_echo, false },
    { "export", op_export, true },   { "unset", op_unset, true }, { "jobs", op_jobs, true },
    { "fg", op_fg, true },           { "bg", op_bg, true },       { "kill", op_kill, true },
    { "history", op_history, true }, { "true", op_true, true },   { "false", op_false, true },
    { ":", op_colon, true },         { "break", op_break, true }, { "continue", op_continue, true },
    { ".", op_dot, true },           { "source", op_dot, true },  { "alias", op_alias, true },
    { "unalias", op_unalias, true }
};

struct builtin_op *get_builtins() {
    return builtin_ops;
}

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
