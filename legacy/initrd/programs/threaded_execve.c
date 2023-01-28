#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

static void *do_thread(void *arg __attribute__((unused))) {
    printf("doing thread\n");
    sleep(2);
    printf("???\n");
    return NULL;
}

int main() {
    for (int i = 0; i < 5; i++) {
        pthread_t tid;
        pthread_create(&tid, NULL, do_thread, NULL);
    }

    sleep(1);
    execl("/bin/sh", "sh", "-c", "sleep 2 && echo 'done'", NULL);
    sleep(2);
    return 127;
}
