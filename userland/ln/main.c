#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

void print_usage(char **argv) {
    fprintf(stderr, "Usage: %s [-fs] [-L|-P] source_file... target_dir\n", argv[0]);
}

int main(int argc, char **argv) {
    bool make_sym = false;

    opterr = 0;
    char opt;
    while ((opt = getopt(argc, argv, ":fsLP")) != -1) {
        switch (opt) {
            case 's':
                make_sym = true;
                break;
            case '?':
                print_usage(argv);
                return 2;
            default:
                print_usage(argv);
                return 2;
        }
    }

    if (optind + 2 > argc) {
        print_usage(argv);
        return 2;
    }

    char *source = argv[optind];
    char *target = argv[argc - 1];

    if (!make_sym) {
        fprintf(stderr, "Hard links are unsupported\n");
        return 1;
    }

    if (symlink(target, source)) {
        perror("symlink");
        return 1;
    }

    return 0;
}