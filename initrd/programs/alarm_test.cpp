#include <signal.h>
#include <string.h>
#include <unistd.h>

static volatile sig_atomic_t value;

int main() {
    signal(SIGALRM, [](int) {
        write(STDOUT_FILENO, "Hello from alarm\n", 17);
        value = 1;
    });

    alarm(2);

    write(STDOUT_FILENO, "Registered\n", strlen("Registered\n"));

    for (;;) {
        if (value) {
            break;
        }
    }

    write(STDOUT_FILENO, "After\n", 6);

    alarm(1);
    alarm(0);

    sleep(2);

    return 0;
}