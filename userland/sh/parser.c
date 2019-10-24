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
    char *fixed_line = calloc(strlen(line) + 1, sizeof(char));

    bool prev_was_blackslash = false;
    bool in_s_quote = false;
    bool in_d_quote = false;
    bool in_b_quote = false;
    bool prev_was_digit = false;
    for (size_t i = 0, fixed_i = 0; line[i] != '\0'; i++) {
        char c = line[i];
        switch (c) {
            case '\\':
                if (!prev_was_blackslash) {
                    prev_was_blackslash = true;
                    fixed_line[fixed_i++] = line[i];
                    continue;
                }
                break;
            case '\'':
                in_s_quote = (prev_was_blackslash || in_d_quote || in_b_quote) ? in_s_quote : !in_s_quote;
                break;
            case '"':
                in_d_quote = (prev_was_blackslash || in_s_quote || in_b_quote) ? in_d_quote : !in_d_quote;
                break;
            case '`':
                in_b_quote = (prev_was_blackslash || in_s_quote || in_d_quote) ? in_b_quote : !in_b_quote;
                break;
            case '>':
            case '<': {
                if (prev_was_blackslash || in_s_quote || in_d_quote || in_b_quote) {
                    break;
                }

                int target_fd = c == '>' ? STDOUT_FILENO : STDIN_FILENO;
                if (prev_was_digit) {
                    target_fd = line[i - 1] - '0';
                    fixed_line[--fixed_i] = '\0'; // Delete digit from fixed_line
                }

                i++;

                int mode = REDIRECT_FILE;
                if (line[i] == '&') {
                    if (!isdigit(line[i + 1])) {
                        return WRDE_SYNTAX;
                    }

                    init_redirection(&simple_command->redirection_info, target_fd, REDIRECT_FD, line[i + 1] - '0');

                    i += 2;
                    break;
                } else if (line[i] == '>') {
                    mode = REDIRECT_APPEND_FILE;
                    i++;
                }

                i += strspn(line + i, " \t\n");
                size_t stop = strcspn(line + i, " \t\n");
                if (stop == 0) {
                    return WRDE_SYNTAX;
                }
                line[i + stop] = '\0';

                init_redirection(&simple_command->redirection_info, target_fd, mode, line + i);
                i += stop;
                break;
            }
            default: {
                break;
            }
        }

        fixed_line[fixed_i++] = line[i];

        if (!prev_was_blackslash && !in_s_quote && !in_d_quote && !in_b_quote && isdigit(line[i])) {
            prev_was_digit = true;
        } else {
            prev_was_digit = false;
        }

        prev_was_blackslash = false;
    }

    int ret = wordexp(fixed_line, &simple_command->we, 0);

    free(fixed_line);
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
    bool in_b_quotes = false;

    for (size_t i = 0;; i++) {
        switch (line[i]) {
            case '\\':
                prev_was_backslash = !prev_was_backslash;
                continue;
            case '\'':
                in_s_quotes = (prev_was_backslash || in_d_quotes || in_b_quotes) ? in_s_quotes : !in_s_quotes;
                break;
            case '"':
                in_d_quotes = (prev_was_backslash || in_s_quotes || in_b_quotes) ? in_d_quotes : !in_d_quotes;
                break;
            case '`':
                in_b_quotes = (prev_was_backslash || in_s_quotes || in_d_quotes) ? in_b_quotes : !in_b_quotes;
                break;
            case '\0':
            case '|':
                if (line[i] == '\0' || (!prev_was_backslash && !in_d_quotes && !in_s_quotes && !in_b_quotes)) {
                    if (split_index >= max) {
                        max += SPLIT_BUF_INC;
                        split = realloc(split, max * sizeof(char*));
                    }

                    split[split_index++] = last;
                    if (i >= line_len) { 
                        goto finish;
                    }

                    line[i++] = '\0';

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