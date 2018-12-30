#ifndef _STDLIB_H
#define _STDLIB_H 1

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

__attribute__((__noreturn__)) 
void abort();

int atexit(void (*)(void));
int atoi(const char*);
void free(void*);
char *getenv(const char*);
void *malloc(size_t);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _STDLIB_H */