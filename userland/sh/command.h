#ifndef _COMMAND_H
#define _COMMAND_H 1

#include <wordexp.h>

struct builtin_op;

#define MAX_REDIRECTIONS 10

enum redirection_method {
    REDIRECT_NONE = 0,
    REDIRECT_FILE,
    REDIRECT_APPEND_FILE,
    REDIRECT_FD
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
    struct redirection_desc redirection_descs[MAX_REDIRECTIONS];
};

enum command_type {
    COMMAND_SIMPLE,
    COMMAND_PIPELINE,
    COMMAND_LIST,
    COMMAND_COMPOUND,
    COMMAND_FUNCTION_DECLARATION
};

enum command_mode {
    COMMAND_FOREGROUND = 0,
    COMMAND_BACKGROUND
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
    enum command_mode mode;
    union {
        struct command_simple simple_command;
        struct command_pipeline pipeline;
        struct command_list list;
        struct command_compound compound;
        struct command_function_declaration function_declaration;
    } command;
};

void init_redirection(struct redirection_info *info,  int target_fd, enum redirection_method method, ...);

struct command *command_construct(enum command_type type, enum command_mode mode, ...);
int command_run(struct command *command);
void command_cleanup(struct command *command);

void command_init_special_vars();

void set_exit_status(int n);

#endif /* _COMMAND_H */