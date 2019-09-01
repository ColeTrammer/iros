#ifndef _STDLIB_H
#define _STDLIB_H 1

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef __is_libk

__attribute__((__noreturn__))
void exit(int status);

int atexit(void (*)(void));
char *getenv(const char*);

#endif

__attribute__((__noreturn__)) 
void abort();

int atoi(const char*);

void *malloc(size_t);
void *calloc(size_t, size_t);
void *realloc(void*, size_t);
void free(void*);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _STDLIB_H */