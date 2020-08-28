#define __libc_internal

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <search.h>
#include <semaphore.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

struct __lock __sem_open_lock = { 0 };
struct __sem *__sem_open_head = NULL;
struct __sem *__sem_open_tail = NULL;

sem_t *sem_open(const char *name, int oflags, ...) {
    oflags &= O_CREAT | O_EXCL;
    if (name[0] != '/') {
        errno = EINVAL;
        return SEM_FAILED;
    }

    size_t name_len = strlen(name + 1);
    if (name_len > NAME_MAX - 4) {
        errno = EINVAL;
        return SEM_FAILED;
    }

    mode_t mode = 0;
    unsigned int value_to_place = 0;
    if (oflags & O_CREAT) {
        va_list args;
        va_start(args, oflags);
        mode = va_arg(args, mode_t);
        value_to_place = va_arg(args, unsigned int);
        va_end(args);
    }

    if (value_to_place > SEM_VALUE_MAX) {
        errno = EINVAL;
        return SEM_FAILED;
    }

    sem_t *ret = SEM_FAILED;
    int sem_fd = -1;
    void *sem_memory = MAP_FAILED;
    int pagesize = sysconf(_SC_PAGE_SIZE);
    __lock(&__sem_open_lock);

    // Duplicate calls to open the same semaphore should return the same sem_t pointer.
    for (struct __sem *sem = __sem_open_head; sem; sem = sem->__sem_open_next) {
        if (strcmp(sem->__sem_open_name, name + 1) == 0) {
            if (oflags & O_EXCL) {
                errno = EEXIST;
                goto error;
            }

            ret = sem;
            atomic_fetch_add(&sem->__sem_ref_count, 1);
            goto done;
        }
    }

    char path[NAME_MAX + sizeof("/dev/shm/sem.")];
    stpcpy(stpcpy(path, "/dev/shm/sem."), name + 1);
    sem_fd = open(path, oflags | O_RDWR, mode);
    if (sem_fd < 0) {
        goto error;
    }

    if (ftruncate(sem_fd, pagesize) < 0) {
        goto error;
    }

    sem_memory = mmap(NULL, pagesize, PROT_READ | PROT_WRITE, MAP_SHARED, sem_fd, 0);
    if (sem_memory == MAP_FAILED) {
        goto error;
    }

    struct __sem *sem = malloc(sizeof(struct __sem) + name_len + 1);
    if (!sem) {
        goto error;
    }
    ret = sem;

    sem->__sem_value = sem_memory;
    sem->__sem_ref_count = 1;
    sem->__sem_flags = __SEM_PSHARED;
    memcpy(sem->__sem_open_name, name + 1, name_len + 1);

    insque(sem, __sem_open_tail);
    __sem_open_tail = sem;
    if (!__sem_open_head) {
        __sem_open_head = sem;
    }

    if (oflags & O_CREAT) {
        *sem->__sem_value = value_to_place;
    }

done:
    __unlock(&__sem_open_lock);
    return ret;

error:
    if (sem_memory != MAP_FAILED) {
        munmap(sem_memory, pagesize);
    }
    if (sem_fd != -1) {
        close(sem_fd);
    }
    __unlock(&__sem_open_lock);
    return ret;
}
