#include <spawn.h>

int posix_spawnp(pid_t *__restrict pidp, const char *__restrict path, const posix_spawn_file_actions_t *fileacts,
                 const posix_spawnattr_t *__restrict attr, char *const args[], char *const envp[]) {
    return __posix_spawn_internal(pidp, path, fileacts, attr, args, envp, 1);
}
