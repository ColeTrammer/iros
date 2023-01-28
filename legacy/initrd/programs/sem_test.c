#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>

static sem_t sem1;
static sem_t *sem2;

static void *do_sem1(void *arg) {
    for (int i = 1; i <= 5; i++) {
        sem_wait(&sem1);
        printf("[%d]: WAITED on sem1\n", i);
        usleep(100000);
    }
    return arg;
}

static void *do_sem2(void *arg) {
    for (int i = 1; i <= 5; i++) {
        sem_wait(sem2);
        printf("[%d]: WAITED on sem2\n", i);
    }
    return arg;
}

int main() {
    sem_init(&sem1, 0, 5);
    sem2 = sem_open("/sem_test", O_EXCL | O_CREAT, 0777, 5);
    if (sem2 == SEM_FAILED) {
        perror("sem_test: sem_open (1)");
        return 1;
    }
    sem_t *test = sem_open("/sem_test", 0);
    if (test == SEM_FAILED) {
        perror("sem_test: sem_open (2)");
        return 1;
    }
    assert(test == sem2);
    sem_close(test);

    pthread_t threads[10];
    for (size_t i = 0; i < sizeof(threads) / sizeof(*threads); i++) {
        void *(*start)(void *) = i % 2 == 0 ? do_sem1 : do_sem2;
        pthread_create(&threads[i], NULL, start, NULL);
    }

    for (int i = 0; i < 20; i++) {
        usleep(200000);
        sem_post(&sem1);
        sem_post(sem2);
    }

    for (size_t i = 0; i < sizeof(threads) / sizeof(*threads); i++) {
        pthread_join(threads[i], NULL);
    }

    if (sem_destroy(&sem1)) {
        perror("sem_test: sem_destroy");
    }
    if (sem_close(sem2)) {
        perror("sem_test: sem_close");
    }
    if (sem_unlink("/sem_test")) {
        perror("sem_test: sem_unlink");
    }
    return 0;
}
