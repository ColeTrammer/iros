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

__CCPP_NORETURN void exit(int exit_code);
int atexit(void (*handler)(void));

#ifdef __CCPP_C11
__CCPP_NORETURN void quick_exit(int exit_code);
int at_quick_exit(void (*handler)(void));
#endif

#ifdef __CCPP_C99
__CCPP_NORETURN void _Exit(int exit_code);
#endif

void* malloc(size_t size);
void* calloc(size_t count, size_t size);
void* realloc(void* pointer, size_t new_size);
void free(void* pointer);

#ifdef __CCPP_C11
void* aligned_alloc(size_t alignment, size_t size);
#endif

int system(char const* command);
char* getenv(char const* name);

int atoi(char const* string);
long atol(char const* string);
#ifdef __CCPP_C99
long long atoll(char const* string);
#endif

long strtol(char const* __CCPP_RESTRICT string, char** __CCPP_RESTRICT end, int radix);
#ifdef __CCPP_C99
long long strtoll(char const* __CCPP_RESTRICT string, char** __CCPP_RESTRICT end, int radix);
#endif

unsigned long strtoul(char const* __CCPP_RESTRICT string, char** __CCPP_RESTRICT end, int radix);
#ifdef __CCPP_C99
unsigned long long strtoull(char const* __CCPP_RESTRICT string, char** __CCPP_RESTRICT end, int radix);
#endif

__CCPP_END_DECLARATIONS
