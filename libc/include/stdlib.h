#ifndef _STDLIB_H
#define _STDLIB_H 1

#include <alloca.h>
#include <stddef.h>

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

#define RAND_MAX 32767

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef __is_libk

void exit(int status) __attribute__((__noreturn__));
void _Exit(int status) __attribute__((__noreturn__));

int atexit(void (*)(void));
char *getenv(const char*);
int setenv(const char *__restrict name, const char *__restrict value, int overwrite);
int unsetenv(const char *name);
int putenv(char *string);

int atexit(void (*f)(void));

void srand(unsigned int seed);
int rand(void);
int rand_r(unsigned int *seedp);

int system(const char *cmd);

#endif /* __is_libk */

typedef struct div_t {
    int quot;
    int rem;
} div_t;

typedef struct ldiv_t {
    long quot;
    long rem;
} ldiv_t;

void abort(void) __attribute__((__noreturn__));

void *bsearch(const void *key, const void *base, size_t nmemb, size_t size, int (*compar)(const void *k1, const void *k2));
void qsort(void *base, size_t nmemb, size_t size, int (*compar)(const void *k1, const void *k2));

int rand(void);
void srand(unsigned int seed);

int system(const char *command);

int abs(int n);
long labs(long n);

div_t div(int num, int den);
ldiv_t ldiv(long num, long den);

int atoi(const char *s);
long atol(const char *s);
long long atoll(const char *s);
double atof(const char *s);

long strtol(const char *__restrict str, char **__restrict endptr, int base);
long long strtoll(const char *__restrict str, char **__restrict endptr, int base);
unsigned long strtoul(const char *__restrict str, char **__restrict endptr, int base);
unsigned long long strtoull(const char *__restrict str, char **__restrict endptr, int base);
double strtod(const char *__restrict str, char **__restrict endptr);

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

#ifdef __libc_internal
void __on_exit();
void init_env();
#endif /* __libc_internal */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _STDLIB_H */