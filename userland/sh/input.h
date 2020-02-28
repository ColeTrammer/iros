#ifndef _INPUT_H
#define _INPUT_H 1

#include <liim/pointers.h>
#include <stdio.h>

#include "sh_token.h"

extern char *buffer;
extern char *line_save;

enum input_mode { INPUT_TTY, INPUT_FILE, INPUT_STRING };

enum class InputResult {
    Eof,
    Empty,
    Error,
    Success,
};

struct string_input_source {
    char *string;
    size_t offset;
    size_t max;
};

struct input_source {
    enum input_mode mode;
    union {
        FILE *tty;
        FILE *file;
        struct string_input_source *string_input_source;
    } source;
};

struct string_input_source *input_create_string_input_source(char *s);

InputResult input_get_line(struct input_source *source, ShValue *command);
void input_cleanup(struct input_source *source);
int do_command_from_source(struct input_source *source);

void init_history();
void print_history();
void write_history();

void __refreshcwd();
char *__getcwd();

#endif /* _INPUT_H */
