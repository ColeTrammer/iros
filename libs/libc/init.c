#define __libc_internal

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

char **environ = NULL;

void initialize_standard_library(int argc, char *argv[], char *envp[]) {
    (void) argc;
    (void) argv;

    environ = envp;

    init_errno();
    init_files();
    init_env();
}