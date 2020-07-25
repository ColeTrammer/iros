#define __libc_internal
#include <dlfcn.h>
#include <liim/string.h>
#include <stdio.h>

#define DL_LOG

extern "C" {
void* dlopen(const char* file, int flags) {
    (void) flags;
#ifdef DL_LOG
    fprintf(stderr, "trying to open file `%s' <LAZY=%d NOW=%d GLOBAL=%d LOCAL=%d>\n", file, !!(flags & RTLD_LAZY), !!(flags & RTLD_NOW),
            !!(flags & RTLD_GLOBAL), !!(flags & RTLD_LOCAL));
#endif /* DL_LOG */

    snprintf(__dl_error, sizeof(__dl_error) - 1, "failed to open `%s'", file);
    __dl_has_error = true;
    return nullptr;
}
}
