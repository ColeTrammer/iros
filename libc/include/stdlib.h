#ifndef _STDLIB_H
#define _STDLIB_H 1

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

#ifndef __is_libk

void exit(int status) __attribute__((__noreturn__));

int atexit(void (*)(void));
char *getenv(const char*);

#endif /* __is_libk */

void abort() __attribute__((__noreturn__)) ;

int atoi(const char*);

void *malloc(size_t);
void *calloc(size_t, size_t);
void *realloc(void*, size_t);
void free(void*);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _STDLIB_H */