#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <wordexp.h>

#include "command.h"
#include "parser.h"

static int parse_simple_command(char *line, struct command_simple *simple_command) {
    wordexp_t we;
    int ret = wordexp(line, &we, 0);

    simple_command->args = calloc(we.we_wordc + 1, sizeof(char*));
    memcpy(simple_command->args, we.we_wordv, we.we_wordc * sizeof(char*));

    return ret;
}

#define SPLIT_BUF_INC 10

static char **split_on_pipe(char *line, size_t *num_split) {
    size_t line_len = strlen(line);
    size_t max = SPLIT_BUF_INC;
    size_t split_index = 0;
    char **split = malloc(max * sizeof(char*));
    char *last = line;

    bool prev_was_backslash = false;
    bool in_d_quotes = false;
    bool in_s_quotes = false;

    for (size_t i = 0;; i++) {
        switch (line[i]) {
            case '\\':
                prev_was_backslash = !prev_was_backslash;
                continue;
            case '\'':
                in_s_quotes = !in_s_quotes;
                break;
            case '"':
                in_d_quotes = !in_d_quotes;
                break;
            case '\0':
            case '|':
                if (line[i] == '\0' || (!prev_was_backslash && !in_d_quotes && !in_s_quotes)) {
                    if (split_index >= max) {
                        max += SPLIT_BUF_INC;
                        split = realloc(split, max * sizeof(char*));
                    }

                    for (int j = i - 1; j >= 0 && isspace(line[j]); j--) {
                        line[j] = '\0';
                    }

                    split[split_index++] = last;
                    if (i >= line_len) { 
                        goto finish;
                    }

                    line[i++] = '\0';
                    for (; i < line_len && isspace(line[i]); i++) {
                        line[i] = '\0';
                    }

                    if (i >= line_len) {
                        goto finish;
                    }
                    last = line + i;
                }
                break;
            default:
                break;
        }

        prev_was_backslash = false;
    }

finish:
    assert(split_index > 0);
    *num_split = split_index;
    return split;
}

struct command *parse_line(char *line, int *error) {
    size_t num_split = 0;
    char **split = split_on_pipe(line, &num_split);
    assert(num_split != 0);

    struct command *command = NULL;
    if (num_split == 1) {
        command = command_construct(COMMAND_SIMPLE);
        *error = parse_simple_command(split[0], &command->command.simple_command);
    } else {
        command = command_construct(COMMAND_PIPELINE, num_split);
        struct command_pipeline pipeline = command->command.pipeline;
        for (size_t i = 0; i < num_split; i++) {
            *error = parse_simple_command(split[i], pipeline.commands + i);
            if (*error) {
                break;
            }
        }
    }

    free(split);
    return command;
}