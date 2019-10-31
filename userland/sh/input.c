#include "input.h"

#include <assert.h>
#include <ctype.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

static struct termios saved_termios = { 0 };

void enable_raw_mode() {
    tcgetattr(STDIN_FILENO, &saved_termios);

    struct termios to_set = saved_termios;

    to_set.c_cflag |= (CS8);
    to_set.c_lflag &= ~(ECHO | ICANON | IEXTEN);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &to_set);
}

// FIXME: this needs to be called when the user presses ^C, or termios must be set before
//        the code ever has a chance to set it
void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_termios);
}

enum line_status {
    DONE,
    UNFINISHED_QUOTE,
    ESCAPED_NEWLINE
};

// Checks whether there are any open quotes or not in the line
static enum line_status get_line_status(char *line, size_t len) {
    bool prev_was_backslash = false;
    bool in_s_quotes = false;
    bool in_d_quotes = false;
    bool in_b_quotes = false;

    for (size_t i = 0; i < len; i++) {
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
            default:
                break;
        }

        prev_was_backslash = false;
    }

    if (prev_was_backslash) {
        return ESCAPED_NEWLINE;
    }

    if (in_s_quotes || in_d_quotes || in_b_quotes) {
        return UNFINISHED_QUOTE;
    }

    return DONE;
}

static char *get_tty_input(FILE *tty) {
    size_t buffer_max = 1024;
    size_t buffer_index = 0;
    size_t buffer_length = 0;
    size_t buffer_min_index = 0;
    char *buffer = malloc(buffer_max);

    for (;;) {
        if (buffer_length + 1 >= buffer_max) {
            buffer_max += 1024;
            buffer = realloc(buffer, buffer_max);
        }

        char c;
        errno = 0;
        int ret = read(fileno(tty), &c, 1);

        if (ret == -1) {
            // We were interrupted
            if (errno = EINTR) {
                buffer_length = 0;
                break;
            } else {
                free(buffer);
                return NULL;
            }
        }

        // User pressed ^D
        if (ret == 0) {
            free(buffer);
            return NULL;
        }

        // Terminal escape sequences
        if (c == '\033') {
            read(fileno(tty), &c, 1);
            if (c != '[') {
                continue;
            }

            read(fileno(tty), &c, 1);
            switch (c) {
                case 'A':
                    // Up arrow
                    continue;
                case 'B':
                    // Down arrow
                    continue;
                case 'C':
                    // Right arrow
                    if (buffer_index < buffer_length) {
                        buffer_index++;
                        write(fileno(tty), "\033[1C", 4);
                    }
                    break;
                case 'D':
                    // Left arrow
                    if (buffer_index > buffer_min_index) {
                        buffer_index--;
                        write(fileno(tty), "\033[1D", 4);
                    }
                    break;
                default:
                    continue;
            }

            continue;
        }


        // Pressed back space
        if (c  == 127) {
            if (buffer_index > buffer_min_index) {
                memmove(buffer + buffer_index - 1, buffer + buffer_index, buffer_length - buffer_index);
                buffer[buffer_length - 1] = ' ';

                buffer_index--;
                write(fileno(tty), "\033[1D\033[s", 7);
                write(fileno(tty), buffer + buffer_index, buffer_length - buffer_index);
                write(fileno(tty), "\033[u", 3);
                buffer[buffer_length--] = '\0';
            }

            continue;
        }

        // Stop once we get to a new line
        if (c == '\n') {
            switch (get_line_status(buffer, buffer_length)) {
                case UNFINISHED_QUOTE:
                    break;
                case ESCAPED_NEWLINE:
                    if (buffer_index == buffer_length) {
                        buffer_index--;
                    }
                    buffer_length--;
                    buffer[buffer_index] = '\0';
                    break;
                case DONE:
                    write(fileno(tty), "\n", 1);
                    goto tty_input_done;
                default:
                    assert(false);
                    break;
            }

            // The line was not finished
            buffer_min_index = buffer_index;
            write(fileno(tty), "\n> ", 3);
            continue;
        }

        if (isprint(c)) {
            if (buffer_index != buffer_length) {
                memmove(buffer + buffer_index + 1, buffer + buffer_index, buffer_length - buffer_index);
            }
            buffer[buffer_index] = c;
            buffer_length++;

            // Make sure to save and restore the cursor position
            write(fileno(tty), "\033[s", 3);
            write(fileno(tty), buffer + buffer_index, buffer_length - buffer_index);
            write(fileno(tty), "\033[u\033[1C", 7);
            buffer_index++;
        }
    }

tty_input_done:
    buffer[buffer_length] = '\0';
    return buffer;
}

static char *get_file_input(FILE *file) {
    int sz = 1024;
    int pos = 0;
    char *buffer = malloc(sz);

    bool prev_was_backslash = false;

    for (;;) {
        int c = fgetc(file);

        /* In a comment */
        if (c == '#' && (pos == 0 || isspace(buffer[pos - 1]))) {
            c = getc(file);
            while (c != EOF && c != '\n') {
                c = fgetc(file);
            }

            break;
        }

        if (c == EOF && pos == 0) {
            free(buffer);
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
            break;
        }

        buffer[pos++] = c;

        if (pos >= sz) {
            sz *= 2;
            buffer = realloc(buffer, sz);
        }
    }

    buffer[pos] = '\0';
    return buffer;
}

static char *get_string_input(struct string_input_source *source) {
    if (source->string[source->offset] == '\0') {
        return NULL;
    }

    // Copy the string and add a \n character so the shell parses it as a line
    char *fixed_command = strdup(source->string);

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
            enable_raw_mode();
            char * res = get_tty_input(source->source.tty);
            disable_raw_mode();
            return res;
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
