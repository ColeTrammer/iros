#ifndef _GLOB_H
#define _GLOB_H 1

#include <bits/size_t.h>

#define GLOB_APPEND   1
#define GLOB_DOOFFS   2
#define GLOB_ERR      4
#define GLOB_MARK     8
#define GLOB_NOCHECK  16
#define GLOB_NOESCAPE 32
#define GLOB_NOSORT   64
#define GLOB_TILDE    128

#define GLOB_ABORTED -1
#define GLOB_NOMATCH -2
#define GLOB_NOSPACE -3

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
    size_t gl_pathc;
    char **gl_pathv;
    size_t gl_offs;
} glob_t;

int glob(const char *__restrict pattern, int flags, int (*errfunc)(const char *epath, int eerrno), glob_t *__restrict pglob);
void globfree(glob_t *pglob);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _GLOB_H */