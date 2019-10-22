#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>

#include "command.h"
#include "parser.h"

static int parse_simple_command(char *line, struct command_simple *simple_command) {
    bool in_quotes = false;
    int err = 0;
    char *token_start = line;
    size_t i = 0;

    int sz = 1024;
    int pos = 0;
    char **tokens = malloc(sz * sizeof(char*));

    for (;;) {
        if (!in_quotes && (isspace(line[i]) || line[i] == '\0')) {
            goto add_token;
        }

        if (line[i] == '\0') {
            break;
        }

        // Handle output redirection
        else if (!in_quotes && line[i] == '>') {
            while (isspace(line[++i]));
            simple_command->redirection_info._stdout = line + i;
            while (!isspace(line[i])) { i++; }
            line[i++] = '\0';
            token_start = line + i;
            continue;
        }

        // Handles input redirection
        else if (!in_quotes && line[i] == '<') {
            while (isspace(line[++i]));
            simple_command->redirection_info._stdin = line + i;
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
        line[i++] = '\0';
        tokens[pos++] = token_start;
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

struct command *parse_line(char *line, int *error) {
    struct command *command = malloc(sizeof(struct command));
    command_init(command, COMMAND_SIMPLE);
    *error = parse_simple_command(line, &command->command.simple_command);

    return command;
}