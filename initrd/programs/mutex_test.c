#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

static pthread_mutex_t mutex;

static void *do_stuff(void *ignore) {
    (void) ignore;

    write(1, "D?\n", 3);

    pthread_mutex_lock(&mutex);

    write(1, "B1\n", 3);
    write(1, "B2\n", 3);

    pthread_exit(NULL);
    pthread_mutex_unlock(&mutex);
    return NULL;
}

int main() {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
    pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);
    pthread_mutex_init(&mutex, &attr);

    pthread_t id;
    pthread_create(&id, NULL, do_stuff, NULL);

    write(1, "C?\n", 3);

    sleep(1);

    int ret = pthread_mutex_lock(&mutex);
    errno = ret;
    perror("m");
    assert(ret == EOWNERDEAD);

    pthread_mutex_unlock(&mutex);

    ret = pthread_mutex_lock(&mutex);
    errno = ret;
    perror("l");
    assert(ret == ENOTRECOVERABLE);
    pthread_mutex_lock(&mutex);

    write(1, "A1\n", 3);
    sleep(1);
    write(1, "A2\n", 3);

    pthread_mutex_unlock(&mutex);

    pthread_join(id, NULL);

    _exit(0);
}
