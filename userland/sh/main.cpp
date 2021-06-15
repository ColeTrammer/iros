#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <setjmp.h>
#include <sh/sh_lexer.h>
#include <sh/sh_parser.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <termios.h>
#include <tinput/file_input_source.h>
#include <tinput/string_input_source.h>
#include <tinput/terminal_input_source.h>
#include <unistd.h>

#include "builtin.h"
#include "command.h"
#include "input.h"
#include "job.h"
#include "sh_state.h"

static struct termios saved_termios;

static void restore_termios() {
    if (isatty(STDIN_FILENO)) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_termios);
    }
}

struct passwd* user_passwd;
struct utsname system_name;

int main(int argc, char** argv) {
    if (uname(&system_name)) {
        perror("uname");
        return 1;
    }

    if (!(user_passwd = getpwuid(getuid()))) {
        perror("getpwuid");
        return 1;
    }

    setenv("USER", user_passwd->pw_name, 1);
    setenv("SHELL", user_passwd->pw_shell, 1);
    setenv("HOME", user_passwd->pw_dir, 1);

    ShRepl repl;

    // Respect -c
    int arg_index = 1;
    UniquePtr<TInput::InputSource> input_source;
    bool string_input = false;
    if (argc > 1 && strcmp(argv[1], "-c") == 0) {
        string_input = true;
        arg_index++;
    } else if (argc > 1 && strcmp(argv[1], "-s") == 0) {
        input_source = make_unique<TInput::FileInputSource>(repl, stdin);
        arg_index++;
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
        if (string_input) {
            fprintf(stderr, "Invalid args\n");
            return 1;
        }

        if (!input_source) {
            ShState::the().set_interactive(true);
            ShState::the().set_notify(true);
            ShState::the().set_vi(true);
            command_init_special_vars(argv[0]);

            input_source = make_unique<TInput::TerminalInputSource>(repl);
        }
    } else if (string_input) {
        input_source = make_unique<TInput::StringInputSource>(repl, argv[arg_index]);

        arg_index++;
        if (arg_index >= argc) {
            command_init_special_vars(argv[0]);
        } else {
            command_init_special_vars(argv[arg_index++]);
        }
    } else if (!input_source) {
        input_source = TInput::FileInputSource::create_from_path(repl, argv[arg_index]);
        if (!input_source) {
            perror("sh");
            return 1;
        }

        command_init_special_vars(argv[arg_index++]);
    } else {
        command_init_special_vars(argv[0]);
    }

    command_push_position_params(PositionArgs(argv + arg_index, MAX(0, argc - arg_index)));

    if (isatty(STDOUT_FILENO)) {
        struct sigaction to_set;
        to_set.sa_flags = 0;
        to_set.sa_handler = SIG_IGN;
        sigaction(SIGTTOU, &to_set, NULL);
        sigaction(SIGWINCH, &to_set, NULL);

        sigset_t sigset;
        sigemptyset(&sigset);
        sigaddset(&sigset, SIGTSTP);
        sigprocmask(SIG_SETMASK, &sigset, NULL);

        tcgetattr(STDOUT_FILENO, &saved_termios);
        atexit(restore_termios);

        if (ShState::the().option_interactive()) {
            String path = user_passwd->pw_dir;
            path += "/.sh_init";
            if (!access(path.string(), F_OK)) {
                char* args[3] = { const_cast<char*>("."), path.string(), nullptr };
                op_dot(args);
            }
        }
    }

    repl.enter(*input_source);

    if (ShState::the().option_interactive()) {
        puts("exit");
    }
    return 0;
}
