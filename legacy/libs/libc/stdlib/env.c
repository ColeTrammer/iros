#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ENV_NUM_BITS_BASE (10 * 64)
#define ENV_BUF_INC       20

extern uint64_t *bitmap;
extern size_t num_bits;
extern size_t num_env_vars;
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
        memset(bitmap + (num_bits - ENV_NUM_BITS_BASE) / 64, 0, ENV_NUM_BITS_BASE / 8);
    }

    bitmap[uint_off] |= (1ULL << bit_off);
}

static void unset_allocated(size_t env_index) {
    size_t bit_off = env_index % 64;
    size_t uint_off = env_index / 64;

    assert(env_index < num_bits);

    bitmap[uint_off] &= ~(1ULL << bit_off);
}

static int env_add(char *s, bool should_set_allocated) {
    if (!is_environ_malloced) {
        max_env_vars = num_env_vars + ENV_BUF_INC;
        // Calloc ensures everything will be null terminated
        char **new_env = calloc(max_env_vars, sizeof(char *));
        if (new_env == NULL) {
            errno = ENOMEM;
            return -1;
        }

        memcpy(new_env, environ, num_env_vars * sizeof(char *));
        environ = new_env;
        is_environ_malloced = true;
    } else if (num_env_vars >= max_env_vars - 1) { // Subtract 1 b/c it has to be NULL terminated
        max_env_vars += ENV_BUF_INC;
        environ = realloc(environ, max_env_vars * sizeof(char *));
        if (environ == NULL) {
            errno = ENOMEM;
            return -1;
        }
    }

    if (should_set_allocated) {
        set_allocated(num_env_vars);
    }

    environ[num_env_vars++] = s;
    environ[num_env_vars] = NULL;
    return 0;
}

static int env_del(size_t index) {
    if (is_allocated(index)) {
        free(environ[index]);
    }

    for (; index < num_env_vars; index++) {
        if (is_allocated(index + 1)) {
            set_allocated(index);
        } else {
            unset_allocated(index);
        }

        environ[index] = environ[index + 1];
    }

    num_env_vars--;
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
    size_t i = 0;
    while (*e != NULL) {
        // Compare up until the `=` character
        if (strncmp(*e, string, equals - string + 1) == 0) {
            if (is_allocated(i)) {
                free(*e);
            }
            unset_allocated(i);
            *e = string;
            return 0;
        }
        e++;
        i++;
    }

    // At this point we know we have to add it to the array
    return env_add(string, false);
}

int setenv(const char *__restrict name, const char *__restrict value, int overwrite) {
    if (name == NULL || strchr(name, '=') != NULL) {
        errno = EINVAL;
        return -1;
    }

    size_t nl = strlen(name);
    char *to_add = malloc(nl + strlen(value) + 2);
    if (to_add == NULL) {
        errno = ENOMEM;
        return -1;
    }

    strcpy(to_add, name);
    strcat(to_add, "=");
    strcat(to_add, value);

    char **e = environ;
    size_t i = 0;
    while (*e != NULL) {
        if (strncmp(*e, to_add, nl + 1) == 0) {
            if (!overwrite) {
                return 0;
            }

            if (is_allocated(i)) {
                free(*e);
            }

            *e = to_add;
            set_allocated(i);
            return 0;
        }
        e++;
        i++;
    }

    return env_add(to_add, true);
}

int unsetenv(const char *name) {
    if (name == NULL || strchr(name, '=') != NULL) {
        errno = EINVAL;
        return -1;
    }

    size_t nl = strlen(name);

    char **e = environ;
    size_t i = 0;
    while (*e != NULL) {
        if (memcmp(*e, name, nl) == 0 && (*e)[nl] == '=') {
            return env_del(i);
        }
        e++;
        i++;
    }

    // Do nothing
    return 0;
}
