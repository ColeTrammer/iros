#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
static pthread_key_t key;

int main() {
    pthread_key_create(&key, [](void *data) {
        pthread_mutex_lock(&m);
        fprintf(stderr, "destructor being called: [ %p ]\n", data);
        pthread_mutex_unlock(&m);
    });

    pthread_t id;
    pthread_create(
        &id, nullptr,
        [](void *) -> void * {
            pthread_setspecific(key, (void *) &m);
            void *data = pthread_getspecific(key);
            pthread_mutex_lock(&m);
            fprintf(stderr, "t2 data: [ %p ]\n", data);
            pthread_mutex_unlock(&m);
            return nullptr;
        },
        nullptr);

    pthread_setspecific(key, (void *) &key);
    void *data = pthread_getspecific(key);

    pthread_mutex_lock(&m);
    fprintf(stderr, "t1 data: [ %p ]\n", data);
    pthread_mutex_unlock(&m);

    pthread_join(id, nullptr);

    pthread_exit(nullptr);
}
