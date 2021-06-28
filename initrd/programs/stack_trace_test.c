#include <assert.h>
#include <stdint.h>

#pragma GCC optimize "O0"

void c() {
    uint64_t reg = 1;
    asm volatile("xor %%rdx, %%rdx" : "=d"(reg) : : "memory");
    assert(reg == 1);
}

void b() {
    c();
}

void a() {
    b();
}

int main() {
    a();

    return 0;
}
