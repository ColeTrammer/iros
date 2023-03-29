#pragma once

#include <ccpp/bits/config.h>
#include <ccpp/bits/size_t.h>

__CCPP_BEGIN_DECLARATIONS

int memcmp(void const* __lhs, void const* __rhs, size_t __count);
void* memcpy(void* __CCPP_RESTRICT __dest, void const* __CCPP_RESTRICT __str, size_t __count);
void* memmove(void* __dest, void const* __str, size_t __count);
void* memset(void* __dest, int __ch, size_t __count);

int strcmp(char const* __lhs, char const* __rhs);
int strncmp(char const* __lhs, char const* __rhs, size_t __count);
char* strcpy(char* __CCPP_RESTRICT __dest, char const* __CCPP_RESTRICT __src);
char* strncpy(char* __CCPP_RESTRICT __dest, char const* __CCPP_RESTRICT __src, size_t __count);
char* strcat(char* __CCPP_RESTRICT __dest, char const* __CCPP_RESTRICT __src);
char* strncat(char* __CCPP_RESTRICT __dest, char const* __CCPP_RESTRICT __src, size_t __count);
char* strchr(char const* __haystack, int __needle);
char* strrchr(char const* __haystack, int __needle);
char* strstr(char const* __haystack, char const* __needle);
size_t strlen(char const* __str);

#if defined(__CCPP_POSIX_EXTENSIONS) || defined(__CCPP_C23)
char* strdup(char const* __src);
char* strndup(char const* __src, size_t __count);
#endif

__CCPP_END_DECLARATIONS
