#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

void do_stuff() {
    write(STDOUT_FILENO, "Hello from a new thread\n", 24);

    while (1)
        ;
}

int main() {
    void *addr = mmap(NULL, 32 * 0x1000, PROT_WRITE | PROT_READ, MAP_ANONYMOUS | MAP_PRIVATE | MAP_STACK, 0, 0);
    assert(addr != MAP_FAILED);

    assert(create_task((uintptr_t) do_stuff, (uintptr_t) addr + 32 * 0x1000) >= 0);

    printf("Hello from the main thread!\n");

    _exit(0);
}
