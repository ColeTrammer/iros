#include <glob.h>
#include <stdio.h>
#include <string.h>

int glob(const char *__restrict pattern, int flags, int (*errfunc)(const char *epath, int eerrno), glob_t *__restrict pglob) {
    fprintf(stderr, "Glob: %s\n", pattern);
    memset(pglob, 0, sizeof(glob_t));

    (void) flags;
    (void) errfunc;
    return GLOB_ABORTED;
}


void globfree(glob_t *pglob) {
    (void) pglob;
}