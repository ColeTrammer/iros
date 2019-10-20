#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#define ENV_NUM_BITS_BASE (10 * 64)
#define ENV_BUF_INC 20

static uint64_t *bitmap = NULL;
static size_t num_bits = ENV_NUM_BITS_BASE;
static size_t num_env_vars = 0;
static size_t max_env_vars = 0;
static bool is_environ_malloced = false;

static bool is_allocated(size_t env_index) {
    size_t bit_off = env_index % 64;
    size_t uint_off = env_index / 64;

    return bitmap[uint_off] & (1ULL << bit_off);
}

static void set_allocated(size_t env_index) {
    size_t bit_off = env_index % 64;
    size_t uint_off = env_index / 64;

    if (env_index >= num_bits) {
        num_bits += ENV_NUM_BITS_BASE;
        bitmap = realloc(bitmap, num_bits / 8);
    }

    bitmap[uint_off] |= (1ULL << bit_off);
}

static int env_add(char *s) {
    if (!is_environ_malloced) {
        max_env_vars = num_env_vars + ENV_BUF_INC;
        // Calloc ensures everything will be null terminated
        char **new_env = calloc(max_env_vars, sizeof(char*));
        if (new_env == NULL) {
            errno = ENOMEM;
            return -1;
        }

        memcpy(new_env, environ, num_env_vars * sizeof(char*));
        environ = new_env;
        is_environ_malloced = true;
    } else if (num_env_vars >= max_env_vars - 1) { // Subtract 1 b/c it has to be NULL terminated
        max_env_vars += ENV_BUF_INC;
        environ = realloc(environ, max_env_vars * sizeof(char*));
        if (environ == NULL) {
            errno = ENOMEM;
            return -1;
        }
    }

    environ[num_env_vars++] = s;
    environ[num_env_vars] = NULL;
    return 0;
}

char *getenv(const char *key) {
    size_t key_len = strlen(key);

    char **env = environ;
    while (*env != NULL) {
        /* Find a match */
        if (memcmp(*env, key, key_len) == 0 && (*env)[key_len] == '=') {
            return *env + key_len + 1;
        }

        env++;
    }

    /* There is no match */
    return NULL;
}

int putenv(char *string) {
    char *equals = strchr(string, '=');

    // This might actually be allowed but it should not ever happen
    if (equals == NULL) {
        errno = EINVAL;
        return -1;
    }

    char **e = environ;
    while (*e != NULL) {
        // Compare up until the `=` character
        if (strncmp(*e, string, equals - string + 1) == 0) {
            *e = string;
            return 0;
        }
        e++;
    }

    // At this point we know we have to add it to the array
    return env_add(string);
}

int setenv(const char *__restrict name, const char *__restrict value, int overwrite) {
    (void) name;
    (void) value;
    (void) overwrite;
    return -1;
}

int unsetenv(const char *name) {
    (void) name;
    return -1;
}

void init_env() {
    char **e = environ;
    while (*e++) {
        num_env_vars++;
    }

    while (num_env_vars > num_bits) {
        num_bits += ENV_NUM_BITS_BASE;
    }

    bitmap = calloc(num_bits / 64, sizeof(uint64_t));
}