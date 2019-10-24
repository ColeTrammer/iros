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
#include "command.h"
#include "input.h"
#include "parser.h"

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
static struct command *command = NULL;
static sigjmp_buf env;
static volatile sig_atomic_t jump_active = 0;

static void on_int(int signo) {
    assert(signo == SIGINT);
    if (!jump_active) {
        return;
    }

    siglongjmp(env, 1);
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

        to_set.sa_flags = 0;
        to_set.sa_handler = SIG_IGN;
        sigaction(SIGTTOU, &to_set, NULL);

        sigset_t sigset;
        sigemptyset(&sigset);
        sigaddset(&sigset, SIGTSTP);
        sigprocmask(SIG_SETMASK, &sigset, NULL);
    }

    command_init_special_vars();

    for (;;) {
        if (sigsetjmp(env, 1) == 1) {
            if (line) { free(line); }
            if (command) { command_cleanup(command); }
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
            break;
        }

        /* Check If The Line Was Empty */
        if (line[0] == '\0') {
            free(line);
            continue;
        }

        int error = 0;
        struct command *command = parse_line(line, &error);

        if (error) {
            free(line);
            command_cleanup(command);
            fprintf(stderr, "Shell parsing error: %d\n", error);
            continue;
        }

        int status = command_run(command);

        free(line);
        command_cleanup(command);

        if (status == SHELL_EXIT) {
            break;
        }
    }

    input_cleanup(&input_source);

    return EXIT_SUCCESS;
}