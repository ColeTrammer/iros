#include <pthread.h>
#include <stdio.h>

static __thread int a;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static void *do_stuff(void *ignore) {
    (void) ignore;

    a = 4;
    pthread_mutex_lock(&mutex);

    printf("T2 a=%d\n", a);

    pthread_mutex_unlock(&mutex);
    return NULL;
}

int main() {
    pthread_t id;
    pthread_create(&id, NULL, do_stuff, NULL);

    a = 5;
    pthread_mutex_lock(&mutex);

    printf("T1 a=%d\n", a);

    pthread_mutex_unlock(&mutex);

    pthread_join(id, NULL);
    return 0;
}