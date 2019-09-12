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
void _Exit(int status) __attribute__((__noreturn__));

int atexit(void (*)(void));
char *getenv(const char*);

#endif /* __is_libk */

void abort() __attribute__((__noreturn__));

int atoi(const char *s);

void *malloc(size_t size);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
void free(void *ptr);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _STDLIB_H */