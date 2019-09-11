#define __libc_internal

#include <errno.h>
#include <stdio.h>

void initialize_standard_library(int argc, char *argv[], int envc, char *envp[]) {
    (void) argc;
    (void) argv;
    (void) envc;
    (void) envp;

    init_errno();
    init_files();
}