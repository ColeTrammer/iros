#ifndef _KERNEL_UTIL_INIT_H
#define _KERNEL_UTIL_INIT_H 1

// #define INIT_DEBUG

#define __ENUMERATE_INIT_LEVELS    \
    __ENUMERATE_INIT_LEVEL(fs)     \
    __ENUMERATE_INIT_LEVEL(driver) \
    __ENUMERATE_INIT_LEVEL(time)   \
    __ENUMERATE_INIT_LEVEL(net)

#define INIT_FUNCTION(name, level) \
    static __attribute__((used)) __attribute__((section(".init.level." #level))) void (*__init_level_##level##_##name)(void) = name

#define __ENUMERATE_INIT_LEVEL(s)           \
    extern char __init_level_##s##_start[]; \
    extern char __init_level_##s##_end[];
__ENUMERATE_INIT_LEVELS
#undef __ENUMERATE_INIT_LEVEL

#define INIT_START(s) (void (**)(void)) __init_level_##s##_start
#define INIT_END(s)   (void (**)(void)) __init_level_##s##_end

#ifdef INIT_DEBUG
#define __INIT_LOG(f, s) debug_log("Calling init function: [ %s, %p ]\n", s, f)
#else
#define __INIT_LOG(f, s) ((void) (f))
#endif /* INIT_DEBUG */

#define INIT_DO_LEVEL(s)                                                      \
    do {                                                                      \
        for (void (**__f)(void) = INIT_START(s); __f != INIT_END(s); __f++) { \
            __INIT_LOG(*__f, "" #s);                                          \
            (*__f)();                                                         \
        }                                                                     \
    } while (0)

#endif /* _KERNEL_UTIL_INIT_H */
