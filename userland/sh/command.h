#ifndef _COMMAND_H
#define _COMMAND_H 1

#include <wordexp.h>

struct builtin_op;

enum redirection_method {
    REDIRECT_NONE = 0,
    REDIRECT_FILE,
    REDIRECT_APPEND_FILE,
    REDIRECT_PIPE
};

struct redirection_desc {
    enum redirection_method method;
    int target_fd;
    union {
        char *file;
        int fd;
    } desc;
};

struct redirection_info {
    struct redirection_desc _stdin;
    struct redirection_desc _stdout;
    struct redirection_desc _stderr;
};

enum command_type {
    COMMAND_SIMPLE,
    COMMAND_PIPELINE,
    COMMAND_LIST,
    COMMAND_COMPOUND,
    COMMAND_FUNCTION_DECLARATION
};

struct command_simple {
    wordexp_t we;
    struct redirection_info redirection_info;
    struct builtin_op *builtin_op;
};

struct command_pipeline {
    struct command_simple *commands;
    size_t num_commands;
};

struct command_list {
};

struct command_compound {
};

struct command_function_declaration {
};

struct command {
    enum command_type type;
    union {
        struct command_simple simple_command;
        struct command_pipeline pipeline;
        struct command_list list;
        struct command_compound compound;
        struct command_function_declaration function_declaration;
    } command;
};

void init_redirection(struct redirection_desc *desc, enum redirection_method method, int target_fd, ...);

struct command *command_construct(enum command_type type, ...);
int command_run(struct command *command);
void command_cleanup(struct command *command);

#endif /* _COMMAND_H */