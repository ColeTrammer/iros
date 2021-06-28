#include <errno.h>
#include <limits.h>
#include <spawn.h>

int posix_spawn_file_actions_addclose(posix_spawn_file_actions_t *acts, int fd) {
    if (fd < 0 || fd >= OPEN_MAX) {
        return EBADF;
    }

    struct __spawn_file_action act = {
        .__type = __SPAWN_FILE_ACTION_CLOSE,
        .__fd0 = fd,
    };
    return __posix_spawn_file_actions_add_internal(acts, &act);
}
