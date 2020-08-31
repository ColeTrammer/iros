set(SOURCES
    semaphore/__sem_wait.c
    semaphore/sem_close.c
    semaphore/sem_destroy.c
    semaphore/sem_getvalue.c
    semaphore/sem_init.c
    semaphore/sem_open.c
    semaphore/sem_post.c
    semaphore/sem_timedwait.c
    semaphore/sem_trywait.c
    semaphore/sem_unlink.c
    semaphore/sem_wait.c
    atfork.c
    attr.c
    barrier.c
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

add_os_static_library(libpthread_static pthread FALSE)
target_compile_definitions(libpthread PRIVATE __is_libc)
