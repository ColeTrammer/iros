#ifndef _COMMAND_H
#define _COMMAND_H 1

#include <wordexp.h>

#include "sh_token.h"

#define MAX_REDIRECTIONS 10

int command_run(ShValue::Program& program);

void command_init_special_vars(int argc, char** argv);

void set_exit_status(int n);
int get_last_exit_status();

void set_break_count(int count);
void set_continue_count(int count);
int get_loop_depth_count();

#endif /* _COMMAND_H */
