#ifndef _STRINGS_H
#define _STRINGS_H 1

#include <bits/locale_t.h>
#include <bits/size_t.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int bcmp(const void *s1, const void *s2, size_t n);
void bcopy(const void *src, void *dest, size_t n);
void bzero(void *p, size_t n);

char *index(const char *s, int c);
char *rindex(const char *s, int c);

int ffs(int i);
int strcasecmp(const char *s1, const char *s2);
int strncasecmp(const char *s1, const char *s2, size_t n);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _STRINGS_H */
