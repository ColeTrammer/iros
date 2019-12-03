#include <assert.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

static pthread_spinlock_t lock;

void do_stuff() {
    sleep(1);
    pthread_spin_lock(&lock);

    write(STDOUT_FILENO, "Hello from a new thread\n", 24);

    pthread_spin_unlock(&lock);

    while (1)
        ;
}

int main() {
    void *addr = mmap(NULL, 32 * 0x1000, PROT_WRITE | PROT_READ, MAP_ANONYMOUS | MAP_PRIVATE | MAP_STACK, 0, 0);
    assert(addr != MAP_FAILED);

    pthread_spin_init(&lock, 0);
    assert(create_task((uintptr_t) do_stuff, (uintptr_t) addr + 32 * 0x1000) >= 0);

    pthread_spin_lock(&lock);
    printf("Hello from the main thread!\n");

    sleep(2);
    pthread_spin_unlock(&lock);
    sleep(1);

    pthread_spin_destroy(&lock);
    _exit(0);
}

// #include <assert.h>
// #include <pthread.h>
// #include <stddef.h>
// #include <stdint.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <sys/mman.h>
// #include <unistd.h>

// void *do_stuff(void *ignore) {
//     (void) ignore;

//     write(STDOUT_FILENO, "Hello from a new thread\n", 24);

//     pthread_exit(NULL);
//     return NULL;
// }

// int main() {
//     pthread_t thread;
//     assert(pthread_create(&thread, NULL, &do_stuff, NULL) == 0);

//     pthread_join(thread, NULL);
//     printf("Hello from the main thread!\n");

//     _exit(0);
// }
