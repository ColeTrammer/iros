set(SOURCES
    atfork.c
    attr.c
    cancel.c
    cond.c
    condattr.c
    getcpuclockid.c
    key.c
    mutex.c
    mutexattr.c
    once.c
    pthread.c
    spinlock.c
)

add_os_library(libpthread pthread FALSE)
