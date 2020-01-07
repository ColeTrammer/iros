#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <setjmp.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include "builtin.h"
#include "command.h"
#include "input.h"
#include "job.h"
#include "sh_lexer.h"
#include "sh_parser.h"
#include "sh_state.h"

static sigjmp_buf env;
static volatile sig_atomic_t jump_active = 0;
static struct termios saved_termios;

static void restore_termios() {
    if (isatty(STDIN_FILENO)) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_termios);
    }
}

static void on_int(int signo) {
    assert(signo == SIGINT);
    if (!jump_active) {
        return;
    }

    siglongjmp(env, 1);
}

int main(int argc, char** argv) {
    struct input_source input_source;

    // Respect -c
    int arg_index = 1;
    bool input_resolved = false;
    if (argc > 1 && strcmp(argv[1], "-c") == 0) {
        input_source.mode = INPUT_STRING;
        arg_index++;
        input_resolved = true;
    } else if (argc > 1 && strcmp(argv[1], "-s") == 0) {
        input_source.mode = INPUT_FILE;
        input_source.source.file = stdin;
        arg_index++;
        input_resolved = true;
    }

    for (; arg_index < argc; arg_index++) {
        char* arg = argv[arg_index];
        if (arg[0] == '+' || arg[0] == '-') {
            bool set = arg[0] == '-';
            bool result = false;
            if (arg[1] == 'o') {
                if (argv[arg_index + 1]) {
                    ShState::the().process_option(argv[arg_index + 1], set);
                    result = true;
                }
            } else {
                result = ShState::the().process_arg(arg);
            }
            if (result) {
                continue;
            }
        }

        break;
    }

    if (arg_index >= argc) {
        if (!input_resolved) {
            input_source.mode = INPUT_TTY;
            input_source.source.tty = stdin;

            ShState::the().set_interactive(true);
            ShState::the().set_notify(true);
            ShState::the().set_vi(true);
            command_init_special_vars(argv[0]);
        } else if (input_source.mode == INPUT_STRING) {
            fprintf(stderr, "Invalid args\n");
            return 1;
        }
    } else if (input_resolved && input_source.mode == INPUT_STRING) {
        input_source.source.string_input_source = input_create_string_input_source(argv[arg_index]);

        arg_index++;
        if (arg_index >= argc) {
            command_init_special_vars(argv[0]);
        } else {
            command_init_special_vars(argv[arg_index++]);
        }
    } else if (!input_resolved) {
        input_source.mode = INPUT_FILE;
        input_source.source.file = fopen(argv[arg_index], "r");
        if (!input_source.source.file) {
            perror("sh");
            return 1;
        }

        command_init_special_vars(argv[arg_index++]);
    } else {
        command_init_special_vars(argv[0]);
    }

    command_push_position_params(PositionArgs(argv + arg_index, MAX(0, argc - arg_index)));

    // ShState::the().dump();

    if (isatty(STDOUT_FILENO)) {
        struct sigaction to_set;
        to_set.sa_handler = &on_int;
        to_set.sa_flags = 0;
        sigaction(SIGINT, &to_set, NULL);

        to_set.sa_flags = 0;
        to_set.sa_handler = SIG_IGN;
        sigaction(SIGTTOU, &to_set, NULL);

        sigset_t sigset;
        sigemptyset(&sigset);
        sigaddset(&sigset, SIGTSTP);
        sigprocmask(SIG_SETMASK, &sigset, NULL);

        tcgetattr(STDOUT_FILENO, &saved_termios);
        atexit(restore_termios);

        char* base = getenv("HOME");
        char* hist_file_name = (char*) ".sh_hist";
        char* hist_file = reinterpret_cast<char*>(malloc(strlen(base) + strlen(hist_file_name) + 2));
        strcpy(hist_file, base);
        strcat(hist_file, "/");
        strcat(hist_file, hist_file_name);

        setenv("HISTFILE", hist_file, 0);
        setenv("HISTSIZE", "100", 0);

        // setenv makes a copy
        free(hist_file);

        init_history();
        atexit(write_history);
    }

    if (sigsetjmp(env, 1) == 1) {
        fprintf(stderr, "^C%c", '\n');
        if (line_save) {
            free(line_save);
        }
        if (buffer) {
            free(buffer);
        }
    }
    jump_active = 1;

    return do_command_from_source(&input_source);
}