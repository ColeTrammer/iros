#ifndef _BITS_CXX_H
#define _BITS_CXX_H 1

#include <bits/size_t.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct exit_function_entry {
    void (*f)(void *closure);
    void *closure;
    void *dso;
};

extern size_t __exit_functions_max;
extern size_t __exit_functions_size;
extern struct exit_function_entry *__exit_functions;

int __cxa_atexit(void (*f)(void *closure), void *closure, void *dso);
void __cxa_finalize(void *dso);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BITS_CXX_H */
