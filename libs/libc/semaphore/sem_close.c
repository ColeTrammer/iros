#define __libc_internal

#include <errno.h>
#include <search.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/mman.h>

int sem_close(sem_t *s) {
    if (s->__sem_flags & __SEM_ANON) {
        errno = EINVAL;
        return -1;
    }

    int ref_count = atomic_fetch_sub(&s->__sem_ref_count, 1);
    if (ref_count == 1) {
        __lock(&__sem_open_lock);
        if (s == __sem_open_tail) {
            __sem_open_tail = NULL;
        }
        if (s == __sem_open_head) {
            __sem_open_head = NULL;
        }
        remque(s);
        __unlock(&__sem_open_lock);

        int ret = munmap(s->__sem_value, sysconf(_SC_PAGE_SIZE));
        free(s);
        return ret;
    }

    return 0;
}
