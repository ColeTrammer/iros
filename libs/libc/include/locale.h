#ifndef _LOCALE_H
#define _LOCALE_H 1

#include <stddef.h>

#define LC_ALL 1
#define LC_COLLATE 2
#define LC_CTYPE 3
#define LC_MESSAGES 4
#define LC_MONETARY 5
#define LC_NUMERIC 6
#define LC_TIME 7

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct lconv {
    char *currency_symbol;
    char *decimal_point;
    char frac_digits;
    char *grouping;
    char *int_curr_symbol;
    char int_frac_digits;
    char int_n_cs_precedes;
    char int_n_sep_by_space;
    char int_n_sign_posn;
    char int_p_cs_precedes;
    char int_p_sep_by_space;
    char int_p_sign_posn;
    char *mon_decimal_point;
    char *mon_grouping;
    char *mon_thousands_sep;
    char *negative_sign;
    char n_cs_precedes;
    char n_sep_by_space;
    char n_sign_posn;
    char *positive_sign;
    char p_cs_precedes;
    char p_sep_by_space;
    char p_sign_posn;
    char *thousands_sep;
};

char *setlocale(int category, const char *locale);
struct lconv *localeconv(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _LOCALE_H */