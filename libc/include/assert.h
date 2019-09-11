#ifndef _ASSERT_H
#define _ASSERT_H 1

#ifdef NDEBUG
#define assert(ignore) ((void) 0)
#else
#   ifdef __is_kernel
#       include <kernel/hal/output.h>
#       define assert(ex) (void)((ex) || (debug_log_assertion(#ex, __FILE__, __LINE__, __func__), 0))
#   else
        /* TODO: Implement */
#       define assert(ignore) ((void) 0)
#   endif /* __is_libk */
#endif /* NDEBUG */

#endif /* _ASSERT_H */