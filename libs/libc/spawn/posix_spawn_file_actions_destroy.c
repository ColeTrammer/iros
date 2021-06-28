#include <spawn.h>
#include <stdlib.h>

int posix_spawn_file_actions_destroy(posix_spawn_file_actions_t *acts) {
    if (acts->__action_vector) {
        for (size_t i = 0; i < acts->__action_count; i++) {
            struct __spawn_file_action *action = &acts->__action_vector[i];
            if (action->__type == __SPAWN_FILE_ACTION_OPEN) {
                free(action->__path);
            }
        }

        free(acts->__action_vector);
    }

    *acts = (posix_spawn_file_actions_t) { 0 };
    return 0;
}
