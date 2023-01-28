#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>

struct tab_info {
    int tab_width;
    int *tabstop_list;
    size_t num_tabstops;
};

struct tab_info *parse_tabstops(char *tabstops) {
    char *end_ptr = NULL;
    char *tabstops_end = tabstops + strlen(tabstops);
    long num_tabs = strtol(tabstops, &end_ptr, 10);
    if (end_ptr == tabstops_end) {
        if (num_tabs <= 0 || num_tabs > INT_MAX || errno == ERANGE) {
            fprintf(stderr, "expand: invalid tab size: `%s'\n", tabstops);
            return NULL;
        }

        struct tab_info *ret = calloc(1, sizeof(struct tab_info));
        ret->tab_width = num_tabs;
        return ret;
    }

    size_t list_length = 0;
    size_t list_max = 20;
    int *list = malloc(list_max * sizeof(int));
    char *item = strtok(tabstops, ", ");
    int prev_value = 0;
    while (item) {
        char *item_end = item + strlen(item);
        long stop = strtol(item, &end_ptr, 10);
        if (end_ptr != item_end || stop <= prev_value || stop > INT_MAX || errno == ERANGE) {
            fprintf(stderr, "expand: invalid tab stop: `%s'\n", item);
            free(list);
            return NULL;
        }

        if (list_length >= list_max) {
            list_max *= 2;
            list = realloc(list, list_max * sizeof(int));
        }

        list[list_length++] = stop;
        prev_value = stop;

        item = strtok(NULL, ", ");
    }

    if (list_length < 2) {
        fprintf(stderr, "expand: tab stop list must have more than two items: `%s'\n", tabstops);
        free(list);
        return NULL;
    }

    struct tab_info *ret = calloc(1, sizeof(struct tab_info));
    ret->num_tabstops = list_length;
    ret->tabstop_list = list;
    return ret;
}

void free_tab_info(struct tab_info *info) {
    free(info->tabstop_list);
}

static int do_expand(FILE *f, const struct tab_info *info) {
    int col_position = 0;
    int c;
    while ((c = fgetc(f)) != EOF) {
        if (c == '\t') {
            int num_spaces = 1;
            if (info->tab_width != 0) {
                num_spaces = info->tab_width - (col_position % info->tab_width);
            } else {
                for (size_t i = 0; i < info->num_tabstops; i++) {
                    if (info->tabstop_list[i] > col_position) {
                        num_spaces = info->tabstop_list[i] - col_position;
                        break;
                    }
                }
            }

            col_position += num_spaces;
            for (int i = 0; i < num_spaces; i++) {
                putchar(' ');
            }
            continue;
        } else if (c == 127) {
            col_position = MAX(col_position - 1, 0);
        } else if (c == '\n' || c == '\r') {
            col_position = 0;
        } else {
            col_position++;
        }
        putchar(c);
    }

    if (ferror(f)) {
        perror("expand: fgetc");
        return 1;
    }

    return 0;
}

static void print_usage_and_exit(const char *s) {
    fprintf(stderr, "Usage: %s [-t tablist] [file...]\n", s);
    exit(2);
}

static char default_tab_stops[] = "8";

int main(int argc, char **argv) {
    char *tabstops_string = default_tab_stops;

    int opt;
    while ((opt = getopt(argc, argv, ":t:")) != -1) {
        switch (opt) {
            case 't':
                tabstops_string = optarg;
                break;
            case ':':
            case '?':
                print_usage_and_exit(*argv);
                break;
        }
    }

    if (optind == argc) {
        argv[argc++] = "-";
    }

    struct tab_info *tabstops = parse_tabstops(tabstops_string);
    if (!tabstops) {
        return 1;
    }

    bool any_failed = false;
    for (; optind < argc; optind++) {
        FILE *f = stdin;
        if (strcmp(argv[optind], "-") != 0) {
            f = fopen(argv[optind], "r");
            if (!f) {
                perror("expand: fopen");
                any_failed = 1;
                continue;
            }
        }

        if (do_expand(f, tabstops)) {
            any_failed = 1;
        }

        if (f == stdin) {
            clearerr(stdin);
        } else if (fclose(f)) {
            perror("expand: fclose");
            any_failed = 1;
        }
    }

    free_tab_info(tabstops);
    free(tabstops);
    return any_failed;
}
