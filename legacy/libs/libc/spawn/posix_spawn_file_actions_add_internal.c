#include <errno.h>
#include <spawn.h>
#include <stdlib.h>
#include <sys/param.h>

#define SPAWN_FILE_ACTIONS_BASE_CAPACITY 32

int __posix_spawn_file_actions_add_internal(posix_spawn_file_actions_t *acts, struct __spawn_file_action *act) {
    if (acts->__action_count >= acts->__action_max) {
        size_t new_max = MAX(SPAWN_FILE_ACTIONS_BASE_CAPACITY, acts->__action_max * 2);
        struct __spawn_file_action *new_actions = realloc(acts->__action_vector, new_max * sizeof(new_actions[0]));
        if (!new_actions) {
            return errno;
        }

        acts->__action_vector = new_actions;
        acts->__action_max = new_max;
    }

    acts->__action_vector[acts->__action_count++] = *act;
    return 0;
}
