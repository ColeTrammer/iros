#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

int main(/* int argc, char **argv */) {
    // assert(argc == 2);

    // int fd = open(argv[1], O_RDONLY);
    // if (fd < 0) {
    //     perror("open");
    //     return 1;
    // }

    // struct stat stat_buf;
    // if (fstat(fd, &stat_buf)) {
    //     perror("stat");
    //     return 1;
    // }

    // void *file = mmap(NULL, stat_buf.st_size, PROT_READ, MAP_SHARED, fd, 0);
    // if (file == MAP_FAILED) {
    //     perror("mmap");
    //     return 1;
    // }

    // char *buffer = calloc(stat_buf.st_size + 1, sizeof(char));
    // if (!buffer) {
    //     perror("malloc");
    //     return 1;
    // }

    // memcpy(buffer, file, stat_buf.st_size);
    // fputs(buffer, stdout);

    // if (munmap(file, stat_buf.st_size)) {
    //     perror("munmap");
    //     return 1;
    // }

    // printf("\n");
    // fflush(stdout);

    pthread_mutex_t *mutex = mmap(NULL, sysconf(_SC_PAGE_SIZE), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, 0, 0);
    if (mutex == MAP_FAILED) {
        perror("mmap(MAP_ANON | MAP_SHARED)");
        return 1;
    }

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);

    pthread_mutex_init(mutex, &attr);

    pthread_mutex_lock(mutex);

    if (fork()) {
        pthread_mutex_lock(mutex);
        printf("C1\n");
        sleep(1);
        pthread_mutex_unlock(mutex);

        _exit(0);
    }

    printf("P1\n");
    sleep(1);
    pthread_mutex_unlock(mutex);

    waitpid(-1, NULL, 0);

    return 0;
}