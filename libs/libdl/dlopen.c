#define __libc_internal
#include <dlfcn.h>
#include <stdio.h>

#define DL_LOG

void* dlopen(const char* file, int flags) {
    (void) flags;
#ifdef DL_LOG
    fprintf(stderr, "trying to open file `%s' <LAZY=%d NOW=%d GLOBAL=%d LOCAL=%d>\n", file, !!(flags & RTLD_LAZY), !!(flags & RTLD_NOW),
            !!(flags & RTLD_GLOBAL), !!(flags & RTLD_LOCAL));
#endif /* DL_LOG */

    __dl_set_error("Cannot load `%s'", file);
    return NULL;
}
