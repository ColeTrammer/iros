#include <assert.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

int main() {
    void *addr = mmap(NULL, 0x5000, PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
    assert(addr != MAP_FAILED);
    memset(addr, 0xFF, 0x5000);

    assert(munmap((void*) ((uintptr_t) addr + 0x1000), 0x2000) == 0);

    memset((void*) ((uintptr_t) addr), 0xBB, 0x1000);
    memset((void*) ((uintptr_t) addr + 0x3000), 0xAA, 0x2000);

    signal(SIGSEGV, [](int) {
        write(1, "munmap worked!\n", 15);
        _exit(0);
    });

    memset((void*) ((uintptr_t) addr + 0x1000), 0xCC, 0x2000);
    assert(false);
}