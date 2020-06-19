#include <assert.h>
#include <errno.h>
#include <ext/parse_mode.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int do_mkdir(char *path, mode_t mode, bool make_parents) {
    if (!make_parents) {
        int ret = mkdir(path, mode);
        if (ret != 0) {
            perror("mkdir");
            return 1;
        }

        return 0;
    }

    int cwd_save = open(".", O_DIRECTORY);
    if (cwd_save == -1) {
        perror("open");
        return 1;
    }

    if (*path == '/') {
        if (chdir("/")) {
            perror("chidr");
            return 1;
        }
    }

    char *component = strtok(path, "/");
    while (component) {
        int ret = mkdir(component, mode);
        if (ret != 0 && errno != EEXIST) {
            perror("mkdir");
            return 1;
        }

        if (chdir(component)) {
            perror("chdir");
            return 1;
        }

        component = strtok(NULL, "/");
    }

    if (fchdir(cwd_save)) {
        perror("fchdir");
        return 1;
    }

    if (close(cwd_save)) {
        perror("close");
        return 1;
    }

    return 0;
}

void print_usage_and_exit(const char *s) {
    fprintf(stderr, "Usage: %s [-p] [-m mode] <dirs...>\n", s);
    exit(2);
}

int main(int argc, char **argv) {
    int opt;
    bool p = false;

    mode_t umask_value = umask(0);
    mode_t mode = 0777 & ~umask_value;
    while ((opt = getopt(argc, argv, ":pm:")) != -1) {
        switch (opt) {
            case 'p':
                p = true;
                break;
            case 'm': {
                auto fancy_mode = Ext::parse_mode(optarg);
                if (!fancy_mode.has_value()) {
                    fprintf(stderr, "%s: failed to parse mode: `%s'\n", *argv, optarg);
                    return 1;
                }
                mode = fancy_mode.value().resolve(mode | S_IFDIR, umask_value);
                break;
            }
            case ':':
            case '?':
                print_usage_and_exit(*argv);
                break;
        }
    }

    if (optind == argc) {
        print_usage_and_exit(*argv);
    }

    for (; optind < argc; optind++) {
        int ret = do_mkdir(argv[optind], mode, p);
        if (ret) {
            return ret;
        }
    }

    return 0;
}
