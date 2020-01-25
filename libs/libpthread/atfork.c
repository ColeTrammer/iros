#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>

extern void (**__prepare_handlers)(void);
extern int __prepare_handlers_installed;
extern int __prepare_handlers_max;

extern void (**__parent_handlers)(void);
extern int __parent_handlers_installed;
extern int __parent_handlers_max;

extern void (**__child_handlers)(void);
extern int __child_handlers_installed;
extern int __child_handlers_max;

#define DEFAULT_FUNCTION_COUNT 20

static int __do_add(void (***fns)(void), int *install, int *max, void (*fn)(void)) {
    if (!*fns || *install >= *max) {
        *max = MAX(*max * 2, DEFAULT_FUNCTION_COUNT);
        *fns = realloc(*fns, *max);
        if (!*fns) {
            return ENOMEM;
        }
    }

    (*fns)[(*install)++] = fn;
    return 0;
}

int pthread_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void)) {
    if (prepare) {
        if (__do_add(&__prepare_handlers, &__prepare_handlers_installed, &__prepare_handlers_max, prepare)) {
            return ENOMEM;
        }
    }

    if (parent) {
        if (__do_add(&__parent_handlers, &__parent_handlers_installed, &__parent_handlers_max, parent)) {
            return ENOMEM;
        }
    }

    if (child) {
        if (__do_add(&__child_handlers, &__child_handlers_installed, &__child_handlers_max, child)) {
            return ENOMEM;
        }
    }

    return 0;
}