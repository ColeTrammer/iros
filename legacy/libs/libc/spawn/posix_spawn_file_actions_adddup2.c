#include <errno.h>
#include <limits.h>
#include <spawn.h>

int posix_spawn_file_actions_adddup2(posix_spawn_file_actions_t *acts, int oldfd, int newfd) {
    if (oldfd < 0 || oldfd >= OPEN_MAX || newfd < 0 || newfd >= OPEN_MAX) {
        return EBADF;
    }

    struct __spawn_file_action act = {
        .__type = __SPAWN_FILE_ACTION_DUP2,
        .__fd0 = oldfd,
        .__fd1 = newfd,
    };
    return __posix_spawn_file_actions_add_internal(acts, &act);
}
