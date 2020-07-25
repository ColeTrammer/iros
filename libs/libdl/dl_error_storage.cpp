#define __libc_internal
#include <dlfcn.h>

extern "C" {
int __dl_has_error;
char __dl_error[256];
}
