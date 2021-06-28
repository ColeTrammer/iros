#include <errno.h>
#include <limits.h>
#include <spawn.h>
#include <string.h>

int posix_spawn_file_actions_addopen(posix_spawn_file_actions_t *__restrict acts, int fd, const char *__restrict path, int oflags,
                                     mode_t mode) {
    if (fd < 0 || fd >= OPEN_MAX) {
        return EBADF;
    }

    char *path_copy = strdup(path);
    if (!path_copy) {
        return errno;
    }

    struct __spawn_file_action act = {
        .__type = __SPAWN_FILE_ACTION_OPEN,
        .__fd0 = fd,
        .__oflags = oflags,
        .__mode = mode,
        .__path = path_copy,
    };
    return __posix_spawn_file_actions_add_internal(acts, &act);
}
