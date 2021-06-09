#include <assert.h>
#include <getopt.h>
#include <stddef.h>
#include <stdio.h>

int main(int argc, char **argv) {
    int did_help;
    struct option opts[] = {
        { .name = "arg1", .has_arg = no_argument, .flag = NULL, .val = 'a' },
        { .name = "help", .has_arg = no_argument, .flag = &did_help, .val = 1 },
        { .name = "target", .has_arg = required_argument, .flag = NULL, .val = 't' },
        { .name = "parallel", .has_arg = optional_argument, .flag = NULL, .val = 'p' },
        { 0 },
    };

    for (;;) {
        int index = -1;
        int opt = getopt_long(argc, argv, "at:p::", opts, &index);
        if (opt == -1) {
            break;
        }

        if (opt == 0) {
            assert(did_help);
            assert(index == 1);
            fprintf(stderr, "Usage: %s [--arg1] [--help] [--target=TARGET] [--parallel=[NPROC]]\n", *argv);
            continue;
        }

        switch (opt) {
            case 'a':
                fprintf(stderr, "Found arg1 (%s)\n", index == 0 ? "long" : "short");
                break;
            case 't':
                fprintf(stderr, "Found target='%s' (%s)\n", optarg, index == 2 ? "long" : "short");
                break;
            case 'p':
                fprintf(stderr, "Found parallel='%s' (%s)\n", optarg, index == 3 ? "long" : "short");
                break;
            default:
                fprintf(stderr, "Opt='%c'\n", opt);
                break;
        }
    }

    for (int i = optind; i < argc; i++) {
        fprintf(stderr, "Positional Argument: %s\n", argv[i]);
    }

    return 0;
}
