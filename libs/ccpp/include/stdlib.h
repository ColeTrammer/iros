#pragma once

#include <ccpp/bits/config.h>
#include <ccpp/bits/null.h>
#include <ccpp/bits/size_t.h>

#ifdef __CCPP_COMPAT
#include <alloca.h>
#endif

__CCPP_BEGIN_DECLARATIONS

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

__CCPP_NORETURN void abort(void);

__CCPP_NORETURN void exit(int __exit_code);
int atexit(void (*__handler)(void));

#ifdef __CCPP_C11
__CCPP_NORETURN void quick_exit(int __exit_code);
int at_quick_exit(void (*__handler)(void));
#endif

#ifdef __CCPP_C99
__CCPP_NORETURN void _Exit(int __exit_code);
#endif

typedef struct {
    int quot;
    int rem;
} div_t;

typedef struct {
    long quot;
    long rem;
} ldiv_t;

#ifdef __CCPP_C99
typedef struct {
    long long quot;
    long long rem;
} lldiv_t;
#endif

div_t div(int __x, int __y);
ldiv_t ldiv(long __x, long __y);
#ifdef __CCPP_C99
lldiv_t lldiv(long long __x, long long __y);
#endif

int abs(int __value);
long labs(long __value);
#ifdef __CCPP_C99
long long llabs(long long __value);
#endif

void* malloc(size_t __size);
void* calloc(size_t __count, size_t __size);
void* realloc(void* __pointer, size_t __new_size);
void free(void* __pointer);

#ifdef __CCPP_C11
void* aligned_alloc(size_t __alignment, size_t __size);
#endif

int system(char const* __command);
char* getenv(char const* __name);

int atoi(char const* __string);
long atol(char const* __string);
#ifdef __CCPP_C99
long long atoll(char const* __string);
#endif

long strtol(char const* __CCPP_RESTRICT __string, char** __CCPP_RESTRICT __end, int __radix);
#ifdef __CCPP_C99
long long strtoll(char const* __CCPP_RESTRICT __string, char** __CCPP_RESTRICT __end, int __radix);
#endif

unsigned long strtoul(char const* __CCPP_RESTRICT __string, char** __CCPP_RESTRICT __end, int __radix);
#ifdef __CCPP_C99
unsigned long long strtoull(char const* __CCPP_RESTRICT __string, char** __CCPP_RESTRICT __end, int __radix);
#endif

double atof(char const* __string);

#ifdef __CCPP_C99
float strtof(char const* __CCPP_RESTRICT __string, char** __CCPP_RESTRICT __end);
#endif
double strtod(char const* __CCPP_RESTRICT __string, char** __CCPP_RESTRICT __end);
#ifdef __CCPP_C99
long double strtold(char const* __CCPP_RESTRICT __string, char** __CCPP_RESTRICT __end);
#endif

#define RAND_MAX 32767
int rand(void);
void srand(unsigned __seed);

void qsort(void* __ptr, size_t __count, size_t __size, int (*__comp)(void const*, void const*));
void* bsearch(void const* __key, void const* __ptr, size_t __count, size_t __size,
              int (*__comp)(void const*, void const*));

__CCPP_END_DECLARATIONS
