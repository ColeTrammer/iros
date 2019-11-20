#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>

int main()
{
    int fd = shm_open("/shm_test", O_RDWR | O_CREAT | O_EXCL, 0666);
    assert(ftruncate(fd, sizeof(uint32_t)) == 0);

    pid_t pid = fork();
    if (pid == 0) {
        close(fd);

        fd = shm_open("/shm_test", O_RDWR, 0);
        assert(fd != -1);

        volatile uint32_t *shared_mem = mmap(NULL, sizeof(uint32_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        assert(shared_mem != MAP_FAILED);

        sleep(1);
        *shared_mem = 0xCAFEBABE;
        sleep(2);

        return 0;
    }

    volatile uint32_t *shared_mem = mmap(NULL, sizeof(uint32_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    assert(shared_mem != MAP_FAILED);

    fprintf(stderr, "Shared mem (before): [ %#.8X ]\n", *shared_mem);
    sleep(2);

    fprintf(stderr, "Shared mem (after) : [ %#.8X ]\n", *shared_mem);
    assert(*shared_mem == 0xCAFEBABE);

    shm_unlink("/shm_test");
    return 0;
}