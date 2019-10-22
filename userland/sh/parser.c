#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <ctype.h>

#include "command.h"
#include "parser.h"

struct command *parse_line(char *line, int *error) {
    // We currently blindly split lines w/o care
    *error = 0;

    struct command *command = malloc(sizeof(struct command));
    command->type = COMMAND_SIMPLE;
    command_init(command);

    bool in_quotes = false;
    char *token_start = line;
    size_t i = 0;
    while (line[i] != '\0') {
        int sz = 1024;
        int pos = 0;
        char **tokens = malloc(sz * sizeof(char*));

        while (line[i] != '\0') {
            if (!in_quotes && (isspace(line[i]))) {
                goto add_token;
            }

            // Handle output redirection
            else if (!in_quotes && line[i] == '>') {
                while (isspace(line[++i]));
                command->command.simple_command.redirection_info._stdout = line + i;
                while (!isspace(line[i])) { i++; }
                line[i++] = '\0';
                token_start = line + i;
                continue;
            }

            // Handles input redirection
            else if (!in_quotes && line[i] == '<') {
                while (isspace(line[++i]));
                command->command.simple_command.redirection_info._stdin = line + i;
                while (!isspace(line[i])) { i++; }
                line[i++] = '\0';
                token_start = line + i;
                continue;
            }

            /* Assumes quote is at beginning of token */
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

            if (pos + 1 >= sz) {
                sz *= 2;
                tokens = realloc(tokens, sz * sizeof(char*));
            }
        }

        if (in_quotes) {
            pos = 0;
            fprintf(stderr, "Shell: %s\n", "Invalid string format");
        }

        tokens[pos] = NULL;
        command->command.simple_command.args = tokens;
    }

    return command;
}