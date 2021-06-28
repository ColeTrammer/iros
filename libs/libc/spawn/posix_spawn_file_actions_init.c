#include <spawn.h>

int posix_spawn_file_actions_init(posix_spawn_file_actions_t *acts) {
    *acts = (posix_spawn_file_actions_t) { 0 };
    return 0;
}
