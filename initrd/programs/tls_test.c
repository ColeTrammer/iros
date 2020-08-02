#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

static __thread int a;
static __thread int b = 3;
static __thread int c = 6;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static void *do_stuff(void *ignore) {
    (void) ignore;

    a = 4;
    pthread_mutex_lock(&mutex);

    printf("T2 a=%d\n", a);

    b = 7;
    c = 8;

    printf("T2 b=%d c=%d\n", b, c);

    pthread_mutex_unlock(&mutex);
    return NULL;
}

int main() {
    printf("Starting...\n");

    pthread_t id;
    pthread_create(&id, NULL, do_stuff, NULL);

    a = 5;
    pthread_mutex_lock(&mutex);

    printf("T1 a=%d\n", a);

    pthread_mutex_unlock(&mutex);

    pthread_join(id, NULL);

    printf("T1 a=%d\n", a);

    printf("T1 b=%d c=%d\n", b, c);
    return 0;
}
