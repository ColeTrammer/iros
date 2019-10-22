#ifndef _PARSER_H
#define _PARSER_H 1

#include <stddef.h>

struct command;

struct command *parse_line(char *line, int *error);

#endif /* _PARSER_H */