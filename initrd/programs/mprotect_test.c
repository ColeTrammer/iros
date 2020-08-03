#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <setjmp.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

static sigjmp_buf buf;
static bool sigsegv_delivered;

static void fault_handler(int signum) {
    assert(signum == SIGSEGV);
    sigsegv_delivered = true;
    siglongjmp(buf, 1);
}

#define CHECK_SIGSEGV              \
    do {                           \
        assert(sigsegv_delivered); \
        sigsegv_delivered = false; \
    } while (0)

int main() {
    signal(SIGSEGV, fault_handler);

    size_t ps = sysconf(_SC_PAGE_SIZE);
    uint8_t *base = mmap(NULL, 10 * ps, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    if (base == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    base[5 * ps] = 42;
    if (mprotect(base + 5 * ps, 5 * ps, PROT_READ)) {
        perror("mprotect");
        return 1;
    }

    if (!sigsetjmp(buf, 1)) {
        base[5 * ps] = 43;
    }
    CHECK_SIGSEGV;
    assert(base[5 * ps] == 42);

    if (mprotect(base + 1 * ps, 1 * ps, PROT_NONE)) {
        perror("mprotect");
        return 1;
    }

    if (!sigsetjmp(buf, 1)) {
        close(base[1 * ps]);
    }
    CHECK_SIGSEGV;

    base[3 * ps] = 124;
    assert(base[3 * ps] == 124);
    if (mmap(base + 3 * ps, 2 * ps, PROT_READ, MAP_ANON | MAP_PRIVATE | MAP_FIXED, -1, 0) != base + 3 * ps) {
        assert(errno);
        perror("mmap");
        return 1;
    }
    assert(base[3 * ps] == 0);
    if (!sigsetjmp(buf, 1)) {
        base[3 * ps] = 125;
    }
    assert(base[3 * ps] == 0);
    CHECK_SIGSEGV;

    if (munmap(base, 10 * ps)) {
        perror("munmap");
        return 1;
    }

    if (!sigsetjmp(buf, 1)) {
        *base = 1;
    }
    CHECK_SIGSEGV;

    printf("Success\n");
    return 0;
}
