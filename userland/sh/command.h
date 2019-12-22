#ifndef _COMMAND_H
#define _COMMAND_H 1

#include <wordexp.h>

#include "sh_token.h"

#define MAX_REDIRECTIONS 10

int command_run(ShValue::Program& program);

void command_init_special_vars();

void set_exit_status(int n);
int get_last_exit_status();

#endif /* _COMMAND_H */
