#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "../builtin.h"

static int op_time(int, char **argv) {
    struct timeval start;
    gettimeofday(&start, nullptr);
    pid_t pid = fork();
    if (pid == 0) {
        execvp(argv[1], argv + 1);
        perror("execv");
        _exit(127);
    } else if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (waitpid(pid, nullptr, 0) < 0) {
        perror("waitpid");
        return 1;
    }

    struct timeval end;
    gettimeofday(&end, nullptr);

    struct tms tm;
    times(&tm);

    double real_seconds = end.tv_sec - start.tv_sec + (end.tv_usec - start.tv_usec) / 1000000.0;
    double user_seconds = (double) tm.tms_cutime / sysconf(_SC_CLK_TCK);
    double sys_seconds = (double) tm.tms_cstime / sysconf(_SC_CLK_TCK);

    int real_minutes = real_seconds / 60.0;
    int user_minutes = user_seconds / 60.0;
    int sys_minutes = sys_seconds / 60.0;

    while (real_seconds > 60.0) {
        real_seconds -= 60.0;
    }
    while (user_seconds > 60.0) {
        user_seconds -= 60.0;
    }
    while (sys_seconds > 60.0) {
        sys_seconds -= 60.0;
    }

    printf("\nreal %4dm%.3fs\nuser %4dm%.3fs\nsys  %4dm%.3fs\n", real_minutes, real_seconds, user_minutes, user_seconds, sys_minutes,
           sys_seconds);

    return 0;
}
SH_REGISTER_BUILTIN(time, op_time);
