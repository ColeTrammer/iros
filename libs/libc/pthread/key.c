#define __libc_internal

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static void (*destructors[PTHREAD_KEYS_MAX])(void *);
static unsigned long allocated_keys_bitmap[PTHREAD_KEYS_MAX / CHAR_BIT / sizeof(unsigned long)];
static pthread_mutex_t allocated_key_mutex = PTHREAD_MUTEX_INITIALIZER;

void pthread_specific_run_destructors(struct thread_control_block *thread) {
    for (int i = 0; i < PTHREAD_DESTRUCTOR_ITERATIONS; i++) {
        bool did_something = false;
        for (size_t j = 0; j < PTHREAD_KEYS_MAX / CHAR_BIT / sizeof(unsigned long); j++) {
            unsigned long bits = allocated_keys_bitmap[j];
            for (size_t k = 0; ~bits && k < CHAR_BIT * sizeof(unsigned long); k++) {
                int index = j * sizeof(unsigned long) + k;
                if (destructors[index] != NULL && thread->pthread_specific_data[index] != NULL) {
                    void *value = thread->pthread_specific_data[index];
                    thread->pthread_specific_data[index] = NULL;
                    destructors[index](value);
                    did_something = true;
                }
            }
        }

        if (!did_something) {
            break;
        }
    }
}

int pthread_key_create(pthread_key_t *key, void (*destructor)(void *k)) {
    if (key == NULL) {
        return EINVAL;
    }

    int ret = 0;
    pthread_mutex_lock(&allocated_key_mutex);

    for (size_t i = 0; i < PTHREAD_KEYS_MAX / CHAR_BIT / sizeof(unsigned long); i++) {
        if (~allocated_keys_bitmap[i]) {
            for (size_t j = 0; i < sizeof(unsigned long) * CHAR_BIT; j++) {
                if (!(allocated_keys_bitmap[i] & (1UL << j))) {
                    allocated_keys_bitmap[i] |= 1UL << j;
                    key->__id = i * sizeof(unsigned long) + j;
                    destructors[key->__id] = destructor;
                    goto key_create_end;
                }
            }
        }
    }

    ret = EAGAIN;

key_create_end:
    pthread_mutex_unlock(&allocated_key_mutex);
    return ret;
}

int pthread_key_delete(pthread_key_t key) {
    if (key.__id == -1) {
        return EINVAL;
    }

    // FIXME: does this need a lock/atomic store?
    destructors[key.__id] = NULL;
    allocated_keys_bitmap[key.__id / sizeof(unsigned long)] &= ~(1UL << (key.__id % sizeof(unsigned long)));

    key.__id = -1;
    return 0;
}

void *pthread_getspecific(pthread_key_t key) {
    if (key.__id < 0 || key.__id >= PTHREAD_KEYS_MAX) {
        return NULL;
    }

    return __get_self()->pthread_specific_data[key.__id];
}

int pthread_setspecific(pthread_key_t key, const void *value) {
    if (key.__id < 0 || key.__id >= PTHREAD_KEYS_MAX) {
        return EINVAL;
    }

    __get_self()->pthread_specific_data[key.__id] = (void *) value;
    return 0;
}
