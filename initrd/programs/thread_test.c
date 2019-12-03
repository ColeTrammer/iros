#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

int main() {
    {
        void *addr = mmap(NULL, 32 * 0x1000, PROT_WRITE | PROT_READ, MAP_ANONYMOUS | MAP_PRIVATE | MAP_STACK, 0, 0);
        printf("New stack: [ %#.16lX ]\n", (uintptr_t) addr);
        assert(addr != MAP_FAILED);
        asm volatile("movq %0, %%rsp" : : "r"(addr + 32 * 0x1000) : "memory");
    }

    printf("Hello From a new stack!\n");
    exit(0);
}
