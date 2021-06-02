#include <assert.h>
#include <pthread.h>
#include <unistd.h>

static void do_thing(void *arg) {
    assert(arg == NULL);

    assert(write(1, "HELLO\n", 6));
}

int main() {
    pthread_cleanup_push(do_thing, NULL);
    pthread_exit(NULL);
    pthread_cleanup_pop(0);
    return 0;
}
