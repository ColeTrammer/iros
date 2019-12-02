#include <assert.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

void on_int(int sig) {
    assert(sig == SIGINT);

    write(1, "INTERRUPT\n", 10);
}

int main() {
    signal(SIGINT, &on_int);
    char c;
    while (1) {
        read(0, &c, 1);
    };
}