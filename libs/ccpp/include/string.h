#pragma once

#include <ccpp/bits/config.h>
#include <ccpp/bits/size_t.h>

__CCPP_BEGIN_DECLARATIONS

void* memcpy(void* __CCPP_RESTRICT dest, void const* __CCPP_RESTRICT str, size_t count);
void* memmove(void* dest, void const* str, size_t count);
void* memset(void* dest, int ch, size_t count);

char* strchr(char const* haystack, int needle);
char* strstr(char const* haystack, char const* needle);

size_t strlen(char const* string);

__CCPP_END_DECLARATIONS
