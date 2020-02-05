#ifndef _REGEX_H
#define _REGEX_H 1

#include <bits/size_t.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef long regoff_t;

typedef struct {
    size_t re_nsub;
    void *__re_compiled_data;
} regex_t;

typedef struct {
    regoff_t rm_so;
    regoff_t rm_eo;
} regmatch_t;

#define REG_EXTENDED 1
#define REG_ICASE    2
#define REG_NOSUB    4
#define REG_NEWLINE  8

#define REG_NOTBOL 16
#define REG_NOTEOL 32

#define REG_NOMATCH  -1
#define REG_BADPAT   -2
#define REG_ECOLLATE -3
#define REG_ECTYPE   -4
#define REG_EESCAPE  -5
#define REG_ESUBREG  -6
#define REG_EBRACK   -7
#define REG_EPAREN   -8
#define REG_EBRACE   -9
#define REG_BADBR    -10
#define REG_ERANGE   -11
#define REG_ESPACE   -12
#define REG_BADRPT   -13

int regcomp(regex_t *__restrict regex, const char *__restrict str, int cflags);
size_t regerror(int error, const regex_t *__restrict regex, char *__restrict buf, size_t buffer_len);
#ifdef __cplusplus
int regexec(const regex_t *__restrict regex, const char *__restrict str, size_t nmatch, regmatch_t __restrict matches[], int eflags);
#else
int regexec(const regex_t *__restrict regex, const char *__restrict str, size_t nmatch, regmatch_t matches[__restrict], int eflags);
#endif /* __cplusplus */
void regfree(regex_t *regex);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _REGEX_H */