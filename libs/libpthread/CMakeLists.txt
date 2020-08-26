set(SOURCES
    atfork.c
    attr.c
    barrierattr.c
    cancel.c
    cond.c
    condattr.c
    getcpuclockid.c
    key.c
    mutex.c
    mutexattr.c
    once.c
    pthread.c
    rwlock.c
    rwlockattr.c
    spinlock.c
)

add_os_library(libpthread pthread FALSE)
target_compile_definitions(libpthread PRIVATE __is_libc)
