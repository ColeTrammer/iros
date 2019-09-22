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

#if (defined(__is_kernel) || defined(__is_libk)) && defined(KERNEL_MALLOC_DEBUG)
void *malloc(size_t size, int line, const char *func);
#define malloc(sz) malloc(sz, __LINE__, __func__)
#else
void *malloc(size_t size);
#endif /* (__is_kernel || __is_libk) && KERNEL_MALLOC_DEBUG */

#if (defined(__is_kernel) || defined(__is_libk)) && defined(KERNEL_MALLOC_DEBUG)
void *calloc(size_t nmemb, size_t size, int line, const char *func);
#define calloc(n, sz) calloc(n, sz, __LINE__, __func__)
#else
void *calloc(size_t nmemb, size_t size);
#endif /* (__is_kernel || __is_libk) && KERNEL_MALLOC_DEBUG */

#if (defined(__is_kernel) || defined(__is_libk)) && defined(KERNEL_MALLOC_DEBUG)
void *realloc(void *ptr, size_t size, int line, const char *func);
#define realloc(ptr, sz) realloc(ptr, sz, __LINE__, __func__)
#else
void *realloc(void *ptr, size_t size);
#endif /* (__is_kernel || __is_libk) && KERNEL_MALLOC_DEBUG */

#if (defined(__is_kernel) || defined(__is_libk)) && defined(KERNEL_MALLOC_DEBUG)
void free(void *ptr, int line, const char *func);
#define free(ptr) free(ptr, __LINE__, __func__)
#else
void free(void *ptr);
#endif /* (__is_kernel || __is_libk) && KERNEL_MALLOC_DEBUG */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _STDLIB_H */