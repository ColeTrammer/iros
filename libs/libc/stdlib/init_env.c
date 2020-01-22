#include <stdlib.h>
#include <unistd.h>

#define ENV_NUM_BITS_BASE (10 * 64)

uint64_t *bitmap = NULL;
size_t num_bits = ENV_NUM_BITS_BASE;
size_t num_env_vars = 0;

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