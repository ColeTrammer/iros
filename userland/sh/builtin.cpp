#include <assert.h>
#include <fcntl.h>
#include <liim/hash_map.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "builtin.h"
#include "command.h"
#include "input.h"
#include "job.h"
#include "sh_state.h"

static int op_exit(char **args) {
    int status = 0;
    if (args[1] != NULL) {
        if (args[2] != NULL) {
            printf("Usage: %s [n]\n", args[0]);
            return 1;
        }
        status = atoi(args[1]);
    }

    exit(status);
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
        fprintf(stderr, "%s: Failed to open file `%s'\n", argv[0], argv[1]);
        return 1;
    }

    int i;
    for (i = 2; argv[i] != NULL; i++)
        ;

    command_push_position_params(PositionArgs(argv + 2, i - 2));

    inc_exec_depth_count();
    int ret = do_command_from_source(&source);
    dec_exec_depth_count();
    return ret;
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

static int op_return(char **argv) {
    int status = 0;
    if (argv[1] != NULL) {
        if (argv[2] != NULL) {
            fprintf(stderr, "Usage: %s [status]\n", argv[0]);
            return 1;
        }

        status = atoi(argv[1]);
    }

    if (get_exec_depth_count() == 0) {
        fprintf(stderr, "Cannot return when not in function or . script\n");
        return 1;
    }

    set_should_return();
    return status;
}

static int op_shift(char **argv) {
    int amount = 1;
    if (argv[1] != NULL) {
        if (argv[2] != NULL) {
            fprintf(stderr, "Usage %s [n]\n", argv[0]);
            return 1;
        }

        amount = atoi(argv[1]);
    }

    if (amount == 1 && command_position_params_size() == 0) {
        return 0;
    }

    if (amount < 0 || amount > (int) command_position_params_size()) {
        fprintf(stderr, "Invalid shift amount\n");
        return 1;
    }

    command_shift_position_params_left(amount);
    return 0;
}

static int op_exec(char **argv) {
    if (argv[1] == NULL) {
        return 0;
    }

    execvp(argv[1], argv + 1);
    if (errno == ENOENT) {
        return 127;
    }
    return 126;
}

static int op_set(char **argv) {
    if (argv[1] == NULL) {
        // FIXME: should print out every shell variable.
        return 0;
    }

    int i = 1;
    for (; argv[i]; i++) {
        if (argv[i][0] == '-' || argv[i][0] == '+') {
            bool worked = false;
            if (argv[i][1] != 'o') {
                worked = ShState::the().process_arg(argv[i]);
            } else {
                bool to_set = argv[i][0] == '-';
                i++;

                if (argv[i]) {
                    ShState::the().process_option(argv[i], to_set);
                    worked = true;
                } else {
                    if (to_set) {
                        ShState::the().dump_for_reinput();
                    } else {
                        ShState::the().dump();
                    }
                    worked = false;
                }
            }

            if (worked) {
                continue;
            }
        }

        break;
    }

    for (; argv[i]; i++) {
        command_add_position_param(argv[i]);
    }

    return 0;
}

static int op_test(char **argv) {
    int argc = 0;
    while (argv[++argc]) {
    }

    if (strcmp(argv[0], "[") == 0) {
        if (strcmp(argv[argc - 1], "]") != 0) {
            fprintf(stderr, "%s: expected `]'\n", argv[0]);
            return 1;
        }

        argc--;
    }

    argc--;
    argv++;

    bool invert = false;
    if (strcmp(argv[0], "!") == 0) {
        invert = true;
        argc--;
        argv++;
    }

    if (argc == 1) {
        return strlen(argv[0]) != 0 ? invert : !invert;
    }

    if (argc == 2) {
        if (strcmp(argv[0], "-b") == 0) {
            struct stat st;
            if (stat(argv[1], &st)) {
                return 1;
            }

            return S_ISBLK(st.st_mode) ? invert : !invert;
        }
        if (strcmp(argv[0], "-c") == 0) {
            struct stat st;
            if (stat(argv[1], &st)) {
                return 1;
            }

            return S_ISCHR(st.st_mode) ? invert : !invert;
        }
        if (strcmp(argv[0], "-d") == 0) {
            struct stat st;
            if (stat(argv[1], &st)) {
                return 1;
            }

            return S_ISDIR(st.st_mode) ? invert : !invert;
        }
        if (strcmp(argv[0], "-e") == 0) {
            struct stat st;
            if (stat(argv[1], &st)) {
                return 1;
            }

            return invert;
        }
        if (strcmp(argv[0], "-f") == 0) {
            struct stat st;
            if (stat(argv[1], &st)) {
                return 1;
            }

            return S_ISREG(st.st_mode) ? invert : !invert;
        }
        if (strcmp(argv[0], "-g") == 0) {
            struct stat st;
            if (stat(argv[1], &st)) {
                return 1;
            }

            return (st.st_mode & 02000) ? invert : !invert;
        }
        if (strcmp(argv[0], "-h") == 0 || strcmp(argv[0], "-L") == 0) {
            struct stat st;
            if (lstat(argv[1], &st)) {
                return 1;
            }

            return S_ISLNK(st.st_mode) ? invert : !invert;
        }
        if (strcmp(argv[0], "-n") == 0) {
            return strlen(argv[1]) != 0 ? invert : !invert;
        }
        if (strcmp(argv[0], "-p") == 0) {
            struct stat st;
            if (stat(argv[1], &st)) {
                return 1;
            }

            return S_ISFIFO(st.st_mode) ? invert : !invert;
        }
        if (strcmp(argv[0], "-r") == 0) {
            if (access(argv[1], R_OK)) {
                return !invert;
            }
            return invert;
        }
        if (strcmp(argv[0], "-S") == 0) {
            struct stat st;
            if (stat(argv[1], &st)) {
                return 1;
            }

            return S_ISSOCK(st.st_mode) ? invert : !invert;
        }
        if (strcmp(argv[0], "-s") == 0) {
            struct stat st;
            if (stat(argv[1], &st)) {
                return 1;
            }

            return st.st_size ? invert : !invert;
        }
        if (strcmp(argv[0], "-t") == 0) {
            return isatty(atoi(argv[1])) ? invert : !invert;
        }
        if (strcmp(argv[0], "-u") == 0) {
            struct stat st;
            if (stat(argv[1], &st)) {
                return 1;
            }

            return (st.st_mode & 04000) ? invert : !invert;
        }
        if (strcmp(argv[0], "-w") == 0) {
            if (access(argv[1], W_OK)) {
                return !invert;
            }
            return invert;
        }
        if (strcmp(argv[0], "-x") == 0) {
            if (access(argv[1], X_OK)) {
                return !invert;
            }
            return invert;
        }
        if (strcmp(argv[0], "-z") == 0) {
            return strlen(argv[1]) == 0 ? invert : !invert;
        }
    }

    if (argc == 3) {
        if (strcmp(argv[1], "-lt") == 0) {
            return atol(argv[0]) < atol(argv[2]) ? invert : !invert;
        }
        if (strcmp(argv[1], "-eq") == 0) {
            return atol(argv[0]) == atol(argv[2]) ? invert : !invert;
        }
        if (strcmp(argv[1], "-gt") == 0) {
            return atol(argv[0]) > atol(argv[2]) ? invert : !invert;
        }
        if (strcmp(argv[1], "-ge") == 0) {
            return atol(argv[0]) >= atol(argv[2]) ? invert : !invert;
        }
        if (strcmp(argv[1], "-le") == 0) {
            return atol(argv[0]) <= atol(argv[2]) ? invert : !invert;
        }
        if (strcmp(argv[1], "==") == 0) {
            return strcmp(argv[0], argv[2]) == 0 ? invert : !invert;
        }
        if (strcmp(argv[1], "!=") == 0) {
            return strcmp(argv[0], argv[2]) != 0 ? invert : !invert;
        }
    }

    return !invert;
}

static struct builtin_op builtin_ops[NUM_BUILTINS] = {
    { "exit", op_exit, true },       { "cd", op_cd, true },         { "echo", op_echo, false },
    { "export", op_export, true },   { "unset", op_unset, true },   { "jobs", op_jobs, true },
    { "fg", op_fg, true },           { "bg", op_bg, true },         { "kill", op_kill, true },
    { "history", op_history, true }, { "true", op_true, true },     { "false", op_false, true },
    { ":", op_colon, true },         { "break", op_break, true },   { "continue", op_continue, true },
    { ".", op_dot, true },           { "source", op_dot, true },    { "alias", op_alias, true },
    { "unalias", op_unalias, true }, { "return", op_return, true }, { "shift", op_shift, true },
    { "exec", op_exec, true },       { "set", op_set, true },       { "[", op_test, true },
    { "test", op_test, true }
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
