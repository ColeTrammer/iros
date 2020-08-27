#ifndef _STDLIB_H
#define _STDLIB_H 1

#include <alloca.h>
#include <bits/locale_t.h>
#include <bits/malloc.h>
#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <sys/wait.h>

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

#define RAND_MAX 32767

#define MB_CUR_MAX 1

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef __is_libk

void exit(int status) __attribute__((__noreturn__));
void _Exit(int status) __attribute__((__noreturn__));

int atexit(void (*f)(void));
char *getenv(const char *key);
int setenv(const char *__restrict name, const char *__restrict value, int overwrite);
int unsetenv(const char *name);
int putenv(char *string);

int atexit(void (*f)(void));

char *realpath(const char *__restrict path, char *resolved_path);

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
int rand_r(unsigned int *seedp);

int system(const char *command);

int posix_openpt(int flags);
char *ptsname(int fd);
int ptsname_r(int fd, char *buf, size_t buflen);
int grantpt(int fd);
int unlockpt(int fd);

int mblen(const char *s, size_t n);
int wctomb(char *s, wchar_t wc);
int mbtowc(wchar_t *pwc, const char *s, size_t n);
size_t wcstombs(char *dest, const wchar_t *src, size_t n);

char *mktemp(char *t);
int mkstemp(char *t);
char *mkdtemp(char *t);

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

int posix_memalign(void **memptr, size_t alignment, size_t size);

#ifdef __libc_internal
void init_env();
#endif /* __libc_internal */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _STDLIB_H */
