#pragma once

#include <ccpp/bits/config.h>

__CCPP_BEGIN_DECLARATIONS

#define LC_ALL      0
#define LC_COLLATE  1
#define LC_CTYPE    2
#define LC_MONETARY 3
#define LC_NUMERIC  4
#define LC_TIME     5

#ifdef __CCPP_POSIX_EXTENSIONS
#define LC_MESSAGES 6
#endif

typedef void* locale_t;

struct lconv {
    char* decimal_point;
    char* thousands_sep;
    char* grouping;

    char* mon_decimal_point;
    char* mon_thousands_sep;
    char* mon_grouping;
    char* positive_sign;
    char* negative_sign;

    char* currency_symbol;
    char frac_digits;
    char p_cs_precedes;
    char n_cs_precedes;
    char p_sep_by_space;
    char n_sep_by_space;
    char p_sign_posn;
    char n_sign_posn;

    char* int_curr_symbol;
    char int_frac_digits;
    char int_p_cs_precedes;
    char int_n_cs_precedes;
    char int_p_sep_by_space;
    char int_n_sep_by_space;
    char int_p_sign_posn;
    char int_n_sign_posn;
};

char* setlocale(int __category, char const* __locale);

struct lconv* localeconv(void);

__CCPP_END_DECLARATIONS
