#ifndef _PARSER_H
#define _PARSER_H 1

#include <stddef.h>

struct command {
    char **args;
    char *_stdin;
    char *_stdout;
    char *_stderr;
    struct builtin_op *builtin_op;
};

void free_commands(struct command **commands);
size_t get_num_commands(struct command **commands);
struct command **split_line(char *line);

#endif /* _PARSER_H */