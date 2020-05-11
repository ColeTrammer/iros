#include <pthread.h>
#include <unistd.h>

static pthread_once_t once = PTHREAD_ONCE_INIT;

void do_thing(void) {
    write(1, "DOING THING\n", 12);
}

void *do_stuff(void *ignore) {
    (void) ignore;

    pthread_once(&once, do_thing);

    write(1, "OTHER THING\n", 12);

    return NULL;
}

int main() {
    pthread_t threads[3];

    pthread_create(threads + 0, NULL, do_stuff, NULL);
    pthread_create(threads + 1, NULL, do_stuff, NULL);
    pthread_create(threads + 2, NULL, do_stuff, NULL);

    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);
    pthread_join(threads[2], NULL);

    pthread_exit(NULL);
}
