#include <pthread.h>
#include <sys/iros.h>

int pthread_getcpuclockid(pthread_t thread, clockid_t *clock_id) {
    return -getcpuclockid(0, thread, clock_id);
}
