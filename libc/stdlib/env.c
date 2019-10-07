#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

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