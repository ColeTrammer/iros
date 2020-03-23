#ifndef _BUILTIN_H
#define _BUILTIN_H 1

#include <stdbool.h>

#define NUM_BUILTINS 31

typedef int (*op_function_t)(char **args);

struct builtin_op {
    char name[16];
    op_function_t op;
    bool run_immediately;
};

struct builtin_op *builtin_find_op(char *name);
bool builtin_should_run_immediately(struct builtin_op *op);
int builtin_do_op(struct builtin_op *op, char **args);

int op_dot(char **argv);

struct builtin_op *get_builtins();

#endif /* _BUILTIN_H */
