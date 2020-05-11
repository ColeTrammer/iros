#ifndef _BITS_MALLOC_H
#define _BITS_MALLOC_H 1

#include <bits/size_t.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if (defined(__is_kernel) || defined(__is_libk)) && defined(KERNEL_MALLOC_DEBUG)
void *malloc(size_t size, int line, const char *func);
#define malloc(sz) malloc(sz, __LINE__, __func__)
#else
void *malloc(size_t size);
#endif /* (__is_kernel || __is_libk) && KERNEL_MALLOC_DEBUG */

#if (defined(__is_kernel) || defined(__is_libk)) && defined(KERNEL_MALLOC_DEBUG)
void *aligned_alloc(size_t alignment, size_t size, int line, const char *func);
#define aligned_alloc(al, sz) aligned_alloc(al, sz, __LINE__, __func__)
#else
void *aligned_alloc(size_t alignment, size_t size);
#endif /* (__is_kernel || __is_libk) && KERNEL_MALLOC_DEBUG */

#if (defined(__is_kernel) || defined(__is_libk)) && defined(KERNEL_MALLOC_DEBUG)
void *calloc(size_t nmemb, size_t size, int line, const char *func);
#define calloc(n, sz) calloc(n, sz, __LINE__, __func__)
#else
void *calloc(size_t nmemb, size_t size);
#endif /* (__is_kernel || __is_libk) && KERNEL_MALLOC_DEBUG */

#if (defined(__is_libk) || defined(__is_kernel)) && (defined(KERNEL_MALLOC_DEBUG) || defined(KERNEL_MEMCPY_DEBUG))
void *realloc(void *ptr, size_t size, int line, const char *func);
#define realloc(ptr, sz) realloc(ptr, sz, __LINE__, __func__)
#else
void *realloc(void *ptr, size_t size);
#endif /* (defined(__is_libk) || defined(__is_kernel)) && (defined(KERNEL_MALLOC_DEBUG) || \
          defined(KERNEL_MEMCPY_DEBUG)) */

#if (defined(__is_kernel) || defined(__is_libk)) && defined(KERNEL_MALLOC_DEBUG)
void free(void *ptr, int line, const char *func);
#define free(ptr) free(ptr, __LINE__, __func__)
#else
void free(void *ptr);
#endif /* (__is_kernel || __is_libk) && KERNEL_MALLOC_DEBUG */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BITS_MALLOC_H */
