#ifndef _COMMAND_H
#define _COMMAND_H 1

struct builtin_op;

enum command_type {
    COMMAND_SIMPLE,
    COMMAND_PIPELINE,
    COMMAND_LIST,
    COMMAND_COMPOUND,
    COMMAND_FUNCTION_DECLARATION
};

struct redirection_info {
    char *_stdin;
    char *_stdout;
    char *_stderr;
};

struct command_simple {
    char **args;
    struct redirection_info redirection_info;
    struct builtin_op *builtin_op;
};

struct command_pipeline {
    struct command_simple *commands;
    size_t num_commands;
    size_t max_commands;
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

void command_init(struct command *command);
int command_run(struct command *command);
void command_cleanup(struct command *command);

#endif /* _COMMAND_H */