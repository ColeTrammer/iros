#ifndef _FNMATCH_H
#define _FNMATCH_H 1

#define FNM_NOMATCH 1

#define FNM_PATHNAME 1
#define FNM_PERIOD   2
#define FNM_NOESCAPE 3

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int fnmatch(const char *pattern, const char *string, int flags);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _FNMATCH_H */