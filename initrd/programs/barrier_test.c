#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

static pthread_barrier_t barrier;

void *do_thread(void *arg __attribute__((unused))) {
    for (size_t i = 0; i < 4; i++) {
        if (pthread_barrier_wait(&barrier) == PTHREAD_BARRIER_SERIAL_THREAD) {
            printf("Round: %lu\n", i);
        }
    }

    return NULL;
}

int main() {
    pthread_barrier_init(&barrier, NULL, 4);

    pthread_t threads[3];
    for (size_t i = 0; i < sizeof(threads) / sizeof(*threads); i++) {
        pthread_create(&threads[i], NULL, do_thread, NULL);
    }

    do_thread(NULL);

    for (size_t i = 0; i < sizeof(threads) / sizeof(*threads); i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_barrier_destroy(&barrier);
    return 0;
}
