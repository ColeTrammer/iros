#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

int main() {
    signal(SIGSEGV, [](int n) {
        write(1, "Page faulted...\n", 16);
        _exit(128 + n);
    });

    *((int*) nullptr) = 0;
    return 0;
}
