#ifndef _STRING_H
#define _STRING_H 1

#include <sys/cdefs.h>

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int memcmp(const void *s1, const void *s2, size_t n);
void *memcpy(void *__restrict dest, const void *__restrict src, size_t n);
void *memmove(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);

size_t strlen(const char *s);
char *strcpy(char *__restrict dest, const char *__restrict src);
char *strncpy(char *__restrict dest, const char *__restrict src, size_t n);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _STRING_H */