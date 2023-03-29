#pragma once

#include <ccpp/bits/config.h>

__CCPP_BEGIN_DECLARATIONS

int isalnum(int __ch);
int isalpha(int __ch);
#ifdef __CCPP_POSIX_EXTENSIONS
int isascii(int __ch);
#endif
int islower(int __ch);
int isupper(int __ch);
int isdigit(int __ch);
int isxdigit(int __ch);
int iscntrl(int __ch);
int isgraph(int __ch);
int isspace(int __ch);
#ifdef __CCPP_C99
int isblank(int __ch);
#endif
int isprint(int __ch);
int ispunct(int __ch);

int tolower(int __ch);
int toupper(int __ch);

__CCPP_END_DECLARATIONS
