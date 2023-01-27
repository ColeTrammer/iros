#pragma once

#include <ccpp/bits/config.h>
#include <ccpp/bits/size_t.h>

__CCPP_BEGIN_DECLARATIONS

char* strchr(char const* haystack, int needle);
char* strstr(char const* haystack, char const* needle);

size_t strlen(char const* string);

__CCPP_END_DECLARATIONS