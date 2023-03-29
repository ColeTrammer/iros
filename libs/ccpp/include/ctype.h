#pragma once

#include <ccpp/bits/config.h>

__CCPP_BEGIN_DECLARATIONS

int isalnum(int ch);
int isalpha(int ch);
#ifdef __CCPP_POSIX_EXTENSIONS
int isascii(int ch);
#endif
int islower(int ch);
int isupper(int ch);
int isdigit(int ch);
int isxdigit(int ch);
int iscntrl(int ch);
int isgraph(int ch);
int isspace(int ch);
#ifdef __CCPP_C99
int isblank(int ch);
#endif
int isprint(int ch);
int ispunct(int ch);

int tolower(int ch);
int toupper(int ch);

__CCPP_END_DECLARATIONS
