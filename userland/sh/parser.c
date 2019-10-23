#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "command.h"
#include "parser.h"

static int parse_simple_command(char *line, struct command_simple *simple_command) {
    bool in_quotes = false;
    int err = 0;
    char *token_start = line;
    size_t i = 0;

    int sz = 100;
    int pos = 0;
    char **tokens = malloc(sz * sizeof(char*));

    for (;;) {
        if ((!in_quotes && isspace(line[i])) || line[i] == '\0') {
            goto add_token;
        }

        if (line[i] == '\0') {
            break;
        }

        // Handle output redirection
        else if (!in_quotes && line[i] == '>') {
            while (isspace(line[++i]));
            init_redirection(&simple_command->redirection_info._stdout, REDIRECT_FILE, STDOUT_FILENO, line + i);
            while (line[i] != '\0' && !isspace(line[i])) { i++; }
            if (line[i] == '\0') {
                break;
            }

            line[i++] = '\0';
            token_start = line + i;
            continue;
        }

        // Handles input redirection
        else if (!in_quotes && line[i] == '<') {
            while (isspace(line[++i]));
            init_redirection(&simple_command->redirection_info._stdin, REDIRECT_FILE, STDIN_FILENO, line + i);
            while (!isspace(line[i])) { i++; }
            line[i++] = '\0';
            token_start = line + i;
            continue;
        }

        // Assumes quote is at beginning of token
        else if (!in_quotes && line[i] == '"') {
            in_quotes = true;
            token_start++;
            i++;
            continue;
        }

        else if (in_quotes && line[i] == '"') {
            in_quotes = false;
            goto add_token;
        }

        else {
            i++;
            continue;
        }

    add_token:
        tokens[pos++] = token_start;
        if (line[i] == '\0') {
            break;
        }

        line[i++] = '\0';
        while (isspace(line[i])) { i++; }
        token_start = line + i;

        if (line[i] == '\0') {
            break;
        }

        if (pos + 1 >= sz) {
            sz *= 2;
            tokens = realloc(tokens, sz * sizeof(char*));
        }
    }

    if (in_quotes) {
        pos = 0;
        err = 1;
        fprintf(stderr, "Shell: %s\n", "Invalid string format");
    }

    tokens[pos] = NULL;
    simple_command->args = tokens;

    return err;
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