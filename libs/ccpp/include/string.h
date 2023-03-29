#pragma once

#include <ccpp/bits/config.h>
#include <ccpp/bits/size_t.h>

__CCPP_BEGIN_DECLARATIONS

int memcmp(void const* lhs, void const* rhs, size_t count);
void* memcpy(void* __CCPP_RESTRICT dest, void const* __CCPP_RESTRICT str, size_t count);
void* memmove(void* dest, void const* str, size_t count);
void* memset(void* dest, int ch, size_t count);

int strcmp(char const* lhs, char const* rhs);
char* strcpy(char* __CCPP_RESTRICT dest, char const* __CCPP_RESTRICT src);
char* strncpy(char* __CCPP_RESTRICT dest, char const* __CCPP_RESTRICT src, size_t count);
char* strcat(char* __CCPP_RESTRICT dest, char const* __CCPP_RESTRICT src);
char* strncat(char* __CCPP_RESTRICT dest, char const* __CCPP_RESTRICT src, size_t count);
char* strchr(char const* haystack, int needle);
char* strrchr(char const* haystack, int needle);
char* strstr(char const* haystack, char const* needle);

size_t strlen(char const* string);

__CCPP_END_DECLARATIONS
