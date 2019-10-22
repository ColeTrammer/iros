#include "input.h"

#include <assert.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>

static char *get_tty_input(FILE *tty) {
    int sz = 1024;
    int pos = 0;
    char *buffer = malloc(sz);

    bool prev_was_backslash = false;

    for (;;) {
        errno = 0;
        int c = fgetc(tty);

        // Means user pressed ^C, so we should go to the next line
        if (c == EOF && errno == EINTR) {
            buffer[0] = '\n';
            buffer[1] = '\0';
            printf("%c", '\n');
            return buffer;
        }

        /* In a comment */
        if (c == '#' && (pos == 0 || isspace(buffer[pos - 1]))) {
            c = getc(tty);
            while (c != EOF && c != '\n') {
                c = fgetc(tty);
            }

            buffer[pos] = '\n';
            buffer[pos + 1] = '\0';
            return buffer;
        }

        if (c == EOF && pos == 0) {
            return NULL;
        }

        if (c == '\n' && prev_was_backslash) {
            buffer[--pos] = '\0';
            prev_was_backslash = false;

            printf("> ");
            fflush(stdout);

            continue;
        }

        if (c == '\\') {
            prev_was_backslash = true;
        } else {
            prev_was_backslash = false;
        }

        if (c == EOF || c == '\n') {
            buffer[pos] = '\n';
            buffer[pos + 1] = '\0';
            return buffer;
        }

        buffer[pos++] = c;

        if (pos + 1 >= sz) {
            sz *= 2;
            buffer = realloc(buffer, sz);
        }
    }
}

static char *get_file_input(FILE *file) {
    int sz = 1024;
    int pos = 0;
    char *buffer = malloc(sz);

    bool prev_was_backslash = false;

    for (;;) {
        errno = 0;
        int c = fgetc(file);

        /* In a comment */
        if (c == '#' && (pos == 0 || isspace(buffer[pos - 1]))) {
            c = getc(file);
            while (c != EOF && c != '\n') {
                c = fgetc(file);
            }

            buffer[pos] = '\n';
            buffer[pos + 1] = '\0';
            return buffer;
        }

        if (c == EOF && pos == 0) {
            return NULL;
        }

        if (c == '\n' && prev_was_backslash) {
            buffer[--pos] = '\0';
            prev_was_backslash = false;
            continue;
        }

        if (c == '\\') {
            prev_was_backslash = true;
        } else {
            prev_was_backslash = false;
        }

        if (c == EOF || c == '\n') {
            buffer[pos] = '\n';
            buffer[pos + 1] = '\0';
            return buffer;
        }

        buffer[pos++] = c;

        if (pos + 1 >= sz) {
            sz *= 2;
            buffer = realloc(buffer, sz);
        }
    }
}

static char *get_string_input(struct string_input_source *source) {
    if (source->string[source->offset] == '\0') {
        return NULL;
    }

    // Copy the string and add a \n character so the shell parses it as a line
    char *fixed_command = malloc(strlen(source->string) + 2);
    strcpy(fixed_command, source->string);
    strcat(fixed_command, "\n");

    // May need to split the command if the string has new lines characters
    source->offset = strlen(source->string);
    return fixed_command;
}

struct string_input_source *input_create_string_input_source(char *s) {
    struct string_input_source *source = malloc(sizeof(struct string_input_source));
    source->offset = 0;
    source->string = s;
    return source;
}

char *input_get_line(struct input_source *source) {
    switch (source->mode) {
        case INPUT_TTY:
            return get_tty_input(source->source.tty);
        case INPUT_FILE:
            return get_file_input(source->source.file);
        case INPUT_STRING:
            return get_string_input(source->source.string_input_source);
        default:
            fprintf(stderr, "Invalid input mode: %d\n", source->mode);
            assert(false);
            break;
    }

    return NULL;
}

void input_cleanup(struct input_source *source) {
    // Close file if necessary
    if (source->mode == INPUT_FILE) {
        fclose(source->source.file);
    } else if (source->mode == INPUT_STRING) {
        free(source->source.string_input_source);
    }
}
