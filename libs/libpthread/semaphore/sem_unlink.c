#define __libc_internal

#include <errno.h>
#include <limits.h>
#include <search.h>
#include <semaphore.h>
#include <string.h>
#include <unistd.h>

int sem_unlink(const char *name) {
    if (name[0] != '/') {
        errno = EINVAL;
        return -1;
    }

    char path[NAME_MAX + sizeof("/dev/shm/sem.")];
    stpcpy(stpcpy(path, "/dev/shm/sem."), name + 1);

    __lock(&__sem_open_lock);
    int ret = unlink(path);
    if (ret == 0) {
        // Remove any existing "cached" semaphore, so that sem_open() can't return it after it has been unlinked.
        for (struct __sem *sem = __sem_open_head; sem; sem = sem->__sem_open_next) {
            if (strcmp(sem->__sem_open_name, name + 1) == 0) {
                if (sem == __sem_open_tail) {
                    __sem_open_tail = NULL;
                }
                if (sem == __sem_open_head) {
                    __sem_open_head = NULL;
                }
                remque(sem);
                break;
            }
        }
    }
    __unlock(&__sem_open_lock);
    return ret;
}
