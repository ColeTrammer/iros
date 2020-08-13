#ifndef _KERNEL_UTIL_MACROS_H
#define _KERNEL_UTIL_MACROS_H 1

#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/param.h>

#define container_of(ptr, type, member) ((type*) ((char*) (ptr) -offsetof(type, member)))

#endif /* _KERNEL_UTIL_MACROS_H */
