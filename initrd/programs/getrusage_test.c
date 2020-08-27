#include <stdio.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

static void print_rusage(struct rusage *rusage) {
    printf("  user time:            %lds %ldus\n"
           "  kernel time:          %lds %ldus\n"
           "  max resident memory:  %ld KB\n"
           "  page faults:          %ld\n"
           "  io faults:            %ld\n"
           "  disk reads:           %ld\n"
           "  disk writes:          %ld\n"
           "  voluntary switches:   %ld\n"
           "  involuntary switches: %ld\n",
           rusage->ru_utime.tv_sec, rusage->ru_utime.tv_usec, rusage->ru_stime.tv_sec, rusage->ru_stime.tv_usec, rusage->ru_maxrss,
           rusage->ru_minflt, rusage->ru_majflt, rusage->ru_inblock, rusage->ru_oublock, rusage->ru_nvcsw, rusage->ru_nivcsw);
}

static int do_child(void) {
    for (int i = 0; i < 1000000; i++) {
        getpid();
    }
    return 0;
}

int main() {
    for (int i = 0; i < 5; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            return do_child();
        }
    }

    do_child();
    do_child();

    while (wait(NULL) != -1)
        ;

    struct rusage self;
    struct rusage children;

    if (getrusage(RUSAGE_SELF, &self) < 0 || getrusage(RUSAGE_CHILDREN, &children) < 0) {
        perror("getrusage_test: getrusage");
        return 1;
    }

    printf("SELF:\n");
    print_rusage(&self);

    printf("CHILDREN:\n");
    print_rusage(&children);

    return 0;
}
