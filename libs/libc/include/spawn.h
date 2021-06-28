#ifndef _SPAWN_H
#define _SPAWN_H 1

#include <bits/mode_t.h>
#include <bits/pid_t.h>
#include <sched.h>
#include <signal.h>

#define POSIX_SPAWN_RESETIDS      1
#define POSIX_SPAWN_SETPGROUP     2
#define POSIX_SPAWN_SETSCHEDPARAM 4
#define POSIX_SPAWN_SETSCHEDULER  8
#define POSIX_SPAWN_SETSIGDEF     16
#define POSIX_SPAWN_SETSIGMASK    32

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

enum __spawn_file_action_type {
    __SPAWN_FILE_ACTION_DUP2,
    __SPAWN_FILE_ACTION_CLOSE,
    __SPAWN_FILE_ACTION_OPEN,
};

struct __spawn_file_action {
    enum __spawn_file_action_type __type;
    int __fd0;
    union {
        int __fd1;
        int __oflags;
    };
    mode_t __mode;
    char *__path;
};

typedef struct {
    struct sched_param __sched_param;
    sigset_t __signal_mask;
    sigset_t __signal_default;
    int __sched_policy;
    pid_t __process_group;
    short __flags;
} posix_spawnattr_t;

typedef struct {
    struct __spawn_file_action *__action_vector;
    size_t __action_count;
    size_t __action_max;
} posix_spawn_file_actions_t;

int __posix_spawn_internal(pid_t *__restrict pidp, const char *__restrict path, const posix_spawn_file_actions_t *fileacts,
                           const posix_spawnattr_t *__restrict attr, char *const args[], char *const envp[], int use_execvpe);
int __posix_spawn_file_actions_add_internal(posix_spawn_file_actions_t *acts, struct __spawn_file_action *act);

int posix_spawn(pid_t *__restrict pidp, const char *__restrict path, const posix_spawn_file_actions_t *fileacts,
                const posix_spawnattr_t *__restrict attr, char *const args[], char *const envp[]);
int posix_spawnp(pid_t *__restrict pidp, const char *__restrict path, const posix_spawn_file_actions_t *fileacts,
                 const posix_spawnattr_t *__restrict attr, char *const args[], char *const envp[]);

int posix_spawn_file_actions_addclose(posix_spawn_file_actions_t *acts, int fd);
int posix_spawn_file_actions_adddup2(posix_spawn_file_actions_t *acts, int oldfd, int newfd);
int posix_spawn_file_actions_addopen(posix_spawn_file_actions_t *__restrict acts, int fd, const char *__restrict path, int oflags,
                                     mode_t mode);
int posix_spawn_file_actions_destroy(posix_spawn_file_actions_t *acts);
int posix_spawn_file_actions_init(posix_spawn_file_actions_t *acts);

int posix_spawnattr_destroy(posix_spawnattr_t *attr);
int posix_spawnattr_getflags(const posix_spawnattr_t *__restrict attr, short *__restrict flags);
int posix_spawnattr_getpgroup(const posix_spawnattr_t *__restrict attr, pid_t *__restrict pidp);
int posix_spawnattr_getschedparam(const posix_spawnattr_t *__restrict attr, struct sched_param *__restrict param);
int posix_spawnattr_getschedpolicy(const posix_spawnattr_t *__restrict attr, int *__restrict policy);
int posix_spawnattr_getsigdefault(const posix_spawnattr_t *__restrict attr, sigset_t *__restrict setp);
int posix_spawnattr_getsigmask(const posix_spawnattr_t *__restrict attr, sigset_t *__restrict setp);
int posix_spawnattr_init(posix_spawnattr_t *attr);
int posix_spawnattr_setflags(posix_spawnattr_t *attr, short flags);
int posix_spawnattr_setpgroup(posix_spawnattr_t *attr, pid_t pgroup);
int posix_spawnattr_setschedparam(posix_spawnattr_t *__restrict attr, const struct sched_param *__restrict param);
int posix_spawnattr_setschedpolicy(posix_spawnattr_t *attr, int policy);
int posix_spawnattr_setsigdefault(posix_spawnattr_t *__restrict attr, const sigset_t *__restrict setp);
int posix_spawnattr_setsigmask(posix_spawnattr_t *__restrict attr, const sigset_t *__restrict setp);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SPAWN_H */
