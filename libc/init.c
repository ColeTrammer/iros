#define __libc_internal

#include <errno.h>
#include <stdio.h>

void initialize_standard_library(int argc, char *argv[], char *envp[]) {
    (void) argc;
    (void) argv;
    (void) envp;

    init_errno();
    init_files();
}