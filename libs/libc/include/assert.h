#ifndef _ASSERT_H
#define _ASSERT_H 1

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern __attribute__((noreturn)) void __assert_failed(const char *exp, const char *file, int line, const char *func);

#ifdef NDEBUG
#define assert(ignore) ((void) 0)
#else
#if defined(__is_kernel) || defined(__is_libk)
#include <kernel/hal/output.h>
#define assert(ex) (void) ((ex) || (debug_log_assertion(#ex, __FILE__, __LINE__, __func__), 0))
#else
#define assert(ex) (void) ((ex) || (__assert_failed(#ex, __FILE__, __LINE__, __func__), 0))
#endif /* __is_kernel || __is_libk */
#endif /* NDEBUG */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _ASSERT_H */