#ifndef _STRINGS_H
#define _STRINGS_H 1

#include <bits/locale_t.h>
#include <bits/size_t.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int ffs(int i);
int strcasecmp(const char *s1, const char *s2);
int strcasencmp(const char *s1, const char *s2, size_t n);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _STRINGS_H */