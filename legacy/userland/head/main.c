#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void print_usage(char **argv) {
    printf("Usage: %s [-n num_lines] [file]\n", argv[0]);
}

int main(int argc, char **argv) {
    int num_lines = 10;
    char opt;
    while ((opt = getopt(argc, argv, ":n:")) != -1) {
        switch (opt) {
            case 'n': {
                if (sscanf(optarg, "%d", &num_lines) != 1) {
                    print_usage(argv);
                    return 0;
                }
                break;
            }
            case ':':
            case '?': {
                print_usage(argv);
                return 0;
            }
            default: {
                abort();
            }
        }
    }

    FILE *file = stdin;
    if (optind == argc - 1) {
        file = fopen(argv[optind], "r");
        if (!file) {
            perror("head");
            return 1;
        }
    } else if (optind < argc) {
        print_usage(argv);
        return 0;
    }

    char *line = NULL;
    size_t line_max = 0;
    for (int i = 0; i < num_lines && getline(&line, &line_max, file) != -1; i++) {
        char *to_append = feof(file) ? "\n" : "";
        printf("%s%s", line, to_append);
    }

    if (file != stdin) {
        fclose(file);
    }

    return 0;
}
