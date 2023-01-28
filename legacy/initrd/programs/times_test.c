#include <stdio.h>
#include <sys/times.h>
#include <sys/wait.h>
#include <unistd.h>

int main() {
    struct tms tms;

    if (fork() == 0) {
        for (int i = 0; i < 10000000; i++) {
            asm volatile(" mov %eax, %eax ");
        }

        for (int i = 0; i < 1000000; i++) {
            getpid();
        }

        _exit(0);
    }

    for (int i = 0; i < 5; i++) {
        clock_t ret = times(&tms);

        printf("times: %lu, %lu, %lu, %lu, %lu\n", ret, tms.tms_utime, tms.tms_stime, tms.tms_cutime, tms.tms_cstime);

        for (int i = 0; i < 1000000; i++) {
            getpid();
        }

        if (i == 3) {
            wait(NULL);
        }
    }

    return 0;
}
