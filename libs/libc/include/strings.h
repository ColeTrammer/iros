#ifndef _STRINGS_H
#define _STRINGS_H 1

#include <locale.h>
#include <sys/types.h>

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