#include <assert.h>
#include <pthread.h>
#include <unistd.h>

int main() {
    pthread_t id;
    pthread_create(
        &id, nullptr,
        [](void*) -> void* {
            pthread_cleanup_push(
                [](void*) {
                    write(1, "CL\n", 3);
                },
                nullptr);
            while (1) {
                write(1, "CD\n", 3);
                pthread_testcancel();
                asm volatile("" ::: "memory");
            }
            assert(false);
            pthread_cleanup_pop(0);
        },
        nullptr);

    sleep(1);

    assert(pthread_cancel(id) == 0);

    void* ret;
    assert(pthread_join(id, &ret) == 0);

    assert(ret == PTHREAD_CANCELED);

    return 0;
}
