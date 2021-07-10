#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <unistd.h>

void print_usage_and_exit(const char *s) {
    fprintf(stderr, "Usage: %s [FIRST] [INCREMENT] LAST\n", s);
    exit(2);
}

int parse_number(const char *s) {
    return atoi(s);
}

int main(int argc, char **argv) {
    int first = 1;
    int increment = 1;
    int last = 1;

    // NOTE: we can't use getopt() because it thinks the number -1 is an option.

    optind = 1;
    int opt_count = argc - optind;
    if (opt_count == 1) {
        last = atoi(argv[optind]);
    } else if (opt_count == 2) {
        first = parse_number(argv[optind]);
        last = parse_number(argv[optind + 1]);
    } else if (opt_count == 3) {
        first = parse_number(argv[optind]);
        increment = parse_number(argv[optind + 1]);
        last = parse_number(argv[optind + 2]);
    } else {
        print_usage_and_exit(*argv);
    }

    if (increment == 0) {
        fprintf(stderr, "seq: increment cannot be 0\n");
        return 1;
    }

    int iter = first;
    int min = MIN(first, last);
    int max = MAX(first, last);
    for (;;) {
        printf("%d\n", iter);

        iter += increment;
        if (iter < min || iter > max) {
            return 0;
        }
    }
}
