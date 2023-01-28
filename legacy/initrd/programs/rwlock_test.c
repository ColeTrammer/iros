#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static pthread_rwlock_t lock = PTHREAD_RWLOCK_INITIALIZER;
static int counter = 42;

static void *do_thread(void *arg __attribute__((unused))) {
    for (int i = 0; i < 5; i++) {
        pthread_rwlock_rdlock(&lock);
        printf("R: counter=%d\n", counter);
        pthread_rwlock_unlock(&lock);
        usleep((rand() % 10000) * 6);
    }
    return NULL;
}

static void *do_write(void *arg __attribute__((unused))) {
    for (int i = 0; i < 5; i++) {
        pthread_rwlock_wrlock(&lock);
        counter = rand() % 100;
        printf("W: counter=%d\n", counter);
        pthread_rwlock_unlock(&lock);
        usleep((rand() % 10000) * 6);
    }
    return NULL;
}

int main() {
    srand(time(NULL));

    pthread_t threads[3];
    for (size_t i = 0; i < sizeof(threads) / sizeof(*threads); i++) {
        pthread_create(&threads[i], NULL, do_thread, NULL);
    }

    pthread_t writer;
    pthread_create(&writer, NULL, do_write, NULL);
    do_write(NULL);

    for (size_t i = 0; i < sizeof(threads) / sizeof(*threads); i++) {
        pthread_join(threads[i], NULL);
    }
    pthread_join(writer, NULL);

    pthread_rwlock_destroy(&lock);
    return 0;
}
