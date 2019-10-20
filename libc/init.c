#define __libc_internal

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stddef.h>
#include <stdlib.h>

char **environ = NULL;

void initialize_standard_library(int argc, char *argv[], char *envp[]) {
    (void) argc;
    (void) argv;
    
    environ = envp;

    init_errno();
    init_files();
    init_env();
}