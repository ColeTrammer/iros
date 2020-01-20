#ifndef _STRING_H
#define _STRING_H 1

#include <bits/locale_t.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if (defined(__is_kernel) || defined(__is_libk)) && defined(KERNEL_MEMCPY_DEBUG)
void *memcpy(void *__restrict dest, const void *__restrict src, size_t n, int line, const char *func);
#define memcpy(d, s, n) memcpy(d, s, n, __LINE__, __func__)
#else
void *memcpy(void *__restrict dest, const void *__restrict src, size_t n);
#endif /* (defined(__is_kernel) || defined(__is_libk)) && defined(KERNEL_MEMCPY_DEBUG) */
#if (defined(__is_kernel) || defined(__is_libk)) && defined(KERNEL_MEMCPY_DEBUG)
void *memmove(void *dest, const void *src, size_t n, int line, const char *func);
#define memmove(d, s, n) memmove(d, s, n, __LINE__, __func__)
#else
void *memmove(void *dest, const void *src, size_t n);
#endif /* #if (defined(__is_kernel) || defined(__is_libk)) && defined(KERNEL_MEMCPY_DEBUG) */
char *strcpy(char *__restrict dest, const char *__restrict src);
char *strncpy(char *__restrict dest, const char *__restrict src, size_t n);
char *stpcpy(char *__restrict dest, const char *__restrict src);
char *stpncpy(char *__restrict dest, const char *__restrict src, size_t n);

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
size_t strnlen(const char *s, size_t n);

char *strdup(const char *s);
char *strndup(const char *s, size_t n);

char *strerror(int errnum);
char *strsignal(int sig);

int strcoll(const char *s1, const char *s2);
size_t strxfrm(char *__restrict s1, const char *__restrict s2, size_t n);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _STRING_H */