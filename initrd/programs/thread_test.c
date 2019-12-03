#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

void *do_stuff(void *ignore) {
    (void) ignore;

    write(STDOUT_FILENO, "Hello from a new thread\n", 24);

    return NULL;
}

int main() {
    pthread_t thread;
    assert(pthread_create(&thread, NULL, &do_stuff, NULL) == 0);

    int ret = pthread_join(thread, NULL);
    assert(ret == 0);

    printf("Hello from the main thread!\n");

    _exit(0);
}
