#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static bool any_failed = false;
static bool all = false;
static char *PATH;

static void display_path(const char *dir, const char *s) {
    size_t dir_len = strlen(dir);
    fputs(dir, stdout);
    if (strrchr(dir, '/') != dir + dir_len - 1) {
        putchar('/');
    }
    puts(s);
}

static void do_which(const char *s) {
    char *path_copy = strdup(PATH);
    char *component = strtok(path_copy, ":");
    bool found_one = false;
    while (component) {
        int dir = open(component, O_DIRECTORY | O_RDONLY);
        if (dir == -1) {
            goto next;
        }

        bool works = false;
        if (faccessat(dir, s, X_OK, 0) == 0) {
            works = true;
        }

        close(dir);

        if (works) {
            display_path(component, s);
            found_one = true;
            if (!all) {
                break;
            }
        }

    next:
        component = strtok(NULL, ":");
    }

    free(path_copy);
    if (!found_one) {
        any_failed = true;
    }
}

void print_usage_and_exit(const char *s) {
    fprintf(stderr, "Usage: %s [-a] <args...>\n", s);
    exit(1);
}

int main(int argc, char **argv) {
    int opt;
    while ((opt = getopt(argc, argv, ":a")) != -1) {
        switch (opt) {
            case 'a':
                all = true;
                break;
            case ':':
            case '?':
                print_usage_and_exit(*argv);
                break;
        }
    }

    PATH = getenv("PATH");
    if (!PATH) {
        return 1;
    }

    if (optind == argc) {
        return 1;
    }

    for (; optind < argc; optind++) {
        do_which(argv[optind]);
    }

    return any_failed;
}
