#ifndef _STRING_H
#define _STRING_H 1

#include <sys/cdefs.h>

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void *memcpy(void *__restrict dest, const void *__restrict src, size_t n);
void *memmove(void *dest, const void *src, size_t n);
char *strcpy(char *__restrict dest, const char *__restrict src);
char *strncpy(char *__restrict dest, const char *__restrict src, size_t n);

char *strcat(char *__restrict dest, const char *__restrict src);
char *strncat(char *__restrict dest, const char *__restrict src, size_t n);

int memcmp(const void *s1, const void *s2, size_t n);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);

void *memchr(const void *s, int c, size_t n);
char *strchr(const char *s, int c);
size_t strcspn(const char *s, const char *reject);
char *strpbrk(const char *s, const char *accept);
char *strrchr(const char *s, int c);
size_t strspn(const char *s, const char *accept);
char *strstr(const char *s, const char *sub);
char *strtok(char *__restrict str, const char *__restrict delim);

void *memset(void *s, int c, size_t n);
size_t strlen(const char *s);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _STRING_H */