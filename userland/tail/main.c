#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void print_usage(char **argv) {
    printf("Usage: %s [-n num_lines] [file]\n", argv[0]);
}

int main(int argc, char **argv) {
    size_t num_lines = 10;
    char opt;
    while ((opt = getopt(argc, argv, ":n:")) != -1) {
        switch (opt) {
            case 'n': {
                if (sscanf(optarg, "%zu", &num_lines) != 1) {
                    print_usage(argv);
                    return 0;
                }
                break;
            }
            case ':':
            case '?': {
                print_usage(argv);
                return 2;
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
            perror("tail");
            return 1;
        }
    } else if (optind < argc) {
        print_usage(argv);
        return 2;
    }

    // TODO: replace array structure with a linked list, it
    //       would be much more efficent
    char **lines = calloc(num_lines, sizeof(char *));
    assert(lines);

    size_t line_num = 0;
    size_t line_max = 0;
    char *line = NULL;
    while (getline(&line, &line_max, file) != -1) {
        if (line_num >= num_lines) {
            free(lines[0]);
            memmove(lines, lines + 1, (num_lines - 1) * sizeof(char *));
            lines[num_lines - 1] = line;
        } else {
            lines[line_num++] = line;
        }

        line = NULL;
        line_max = 0;
    }

    for (size_t i = 0; i < line_num; i++) {
        fputs(lines[i], stdout);
        free(lines[i]);
    }

    free(lines);
    fclose(file);

    return 0;
}
