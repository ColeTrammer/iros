#include <signal.h>
#include <unistd.h>

int main() 
{
    signal(SIGALRM, [](int) {
        write(STDOUT_FILENO, "Hello from alarm\n", 17);
    });

    alarm(2);

    write(STDOUT_FILENO, "After\n", 6);

    return 0;
}