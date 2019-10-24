#ifndef _WORDEXP_H
#define _WORDEXP_H 1

#include <stddef.h>

#define WRDE_APPEND  1
#define WRDE_DOOFFS  2
#define WRDE_NOCMD   4
#define WRDE_REUSE   8
#define WRDE_SHOWERR 16
#define WRDE_UNDEF   32

#define WRDE_BADCHAR -1
#define WRDE_BADVAL -2
#define WRDE_CMDSUB -3
#define WRDE_NOSPACE -4
#define WRDE_SYNTAX -5

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
    size_t we_wordc;
    char **we_wordv;
    size_t we_offs;
} wordexp_t;

int wordexp(const char *s ,wordexp_t *p, int flags);
void wordfree(wordexp_t *p);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _WORDEXP_H */