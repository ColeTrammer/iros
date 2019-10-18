#include <glob.h>

int glob(const char *__restrict pattern, int flags, int (*errfunc)(const char *epath, int eerrno), glob_t *__restrict pglob) {
    (void) pattern;
    (void) flags;
    (void) errfunc;
    (void) pglob;
    return GLOB_ABORTED;
}


void globfree(glob_t *pglob) {
    (void) pglob;
}