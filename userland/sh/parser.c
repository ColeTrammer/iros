#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <ctype.h>

#include "parser.h"

void free_commands(struct command **commands) {
    size_t i = 0;
    struct command *command = commands[i++];
    while (command != NULL) {
        free(command->args);
        free(command);

        command = commands[i++];
    }

    free(commands);
}

size_t get_num_commands(struct command **commands) {
    size_t i = 0;
    struct command *command = commands[i++];
    while (command != NULL) {
        command = commands[i++];
    }

    return i - 1;
}

struct command **split_line(char *line) {
    size_t max_commands = 10;
    struct command **commands = calloc(max_commands, sizeof(struct command*));
    struct command *command;

    bool in_quotes = false;
    char *token_start = line;
    size_t i = 0;
    size_t j = 0;
    while (line[i] != '\0') {
        int sz = 1024;
        int pos = 0;
        char **tokens = malloc(sz * sizeof(char*));

        command = malloc(sizeof(struct command));
        command->_stderr = NULL;
        command->_stdout = NULL;
        command->_stdin = NULL;
        command->builtin_op = NULL;

        if (j >= max_commands - 1) {
            max_commands *= 2;
            commands = realloc(commands, max_commands * sizeof(struct command*));
        }

        commands[j++] = command;

        while (line[i] != '\0') {
            if (!in_quotes && (isspace(line[i]))) {
                goto add_token;
            }

            /* Handle pipes */
            else if (!in_quotes && line[i] == '|') {
                while (isspace(line[++i]));
                token_start = line + i;
                break;
            }

            /* Handle output redirection */
            else if (!in_quotes && line[i] == '>') {
                while (isspace(line[++i]));
                command->_stdout = line + i;
                while (!isspace(line[i])) { i++; }
                line[i++] = '\0';
                token_start = line + i;
                continue;
            }

            /* Handles input redirection */
            else if (!in_quotes && line[i] == '<') {
                while (isspace(line[++i]));
                command->_stdin = line + i;
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
        command->args = tokens;
    }

    commands[j] = NULL;
    return commands;
}