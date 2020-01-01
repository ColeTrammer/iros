#ifndef _WORDEXP_H
#define _WORDEXP_H 1

#ifndef USERLAND_NATIVE
#include <bits/size_t.h>
#else
typedef unsigned long size_t;
#endif /* USERLAND_NATIVE */

#define WRDE_APPEND  1
#define WRDE_DOOFFS  2
#define WRDE_NOCMD   4
#define WRDE_REUSE   8
#define WRDE_SHOWERR 16
#define WRDE_UNDEF   32

#define WRDE_BADCHAR -1
#define WRDE_BADVAL  -2
#define WRDE_CMDSUB  -3
#define WRDE_NOSPACE -4
#define WRDE_SYNTAX  -5

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef _OS_2_SOURCE

#define WRDE_SPECIAL 64
#define WRDE_NOFS    128
#define WRDE_NOGLOB  256

#define WRDE_SPECIAL_AT     0
#define WRDE_SPECIAL_STAR   1
#define WRDE_SPECIAL_POUND  2
#define WRDE_SPECIAL_QUEST  3
#define WRDE_SPECIAL_MINUS  4
#define WRDE_SPECIAL_DOLLAR 5
#define WRDE_SPECIAL_EXCLAM 6
#define WRDE_SPECIAL_ZERO   7
#define WRDE_NUM_SPECIAL    8

typedef struct {
    char *vals[WRDE_NUM_SPECIAL];
} word_special_t;

int we_expand(const char *s, int flags, char **result, word_special_t *special);
int we_unescape(char **s);

#endif /* _OS_2_SOURCE */

typedef struct {
    size_t we_wordc;
    char **we_wordv;
    size_t we_offs;
#ifdef _OS_2_SOURCE
    word_special_t *we_special_vars;
#endif /* _OS_2_SOURCE */
} wordexp_t;

int wordexp(const char *s, wordexp_t *p, int flags);
void wordfree(wordexp_t *p);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _WORDEXP_H */