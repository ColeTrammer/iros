#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

static pthread_t child;

int main() {
    signal(SIGUSR1, [](int) {
        assert(pthread_self() == child);
        assert(write(1, "HELLO\n", 6));
        pthread_exit(NULL);
    });

    assert(pthread_create(
               &child, NULL,
               [](void*) -> void* {
                   while (1) {
                       asm volatile("");
                   }
               },
               NULL) == 0);

    pthread_kill(child, SIGUSR1);
    assert(pthread_join(child, NULL) == 0);
    _exit(0);
}
