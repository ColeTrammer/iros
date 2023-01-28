#ifndef _SYS_SHM_H
#define _SYS_SHM_H 1

#include <bits/pid_t.h>
#include <bits/size_t.h>
#include <bits/time_t.h>
#include <sys/ipc.h>

#define SHM_RDONLY 1
#define SHM_RND    2
#define SHMLBA     4

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef unsigned int shmatt_t;

struct shmid_ds {
    struct ipc_perm shm_perm;
    size_t shm_segsz;
    pid_t shm_lpid;
    pid_t shm_cpid;
    shmatt_t smh_nattch;
    time_t shm_atime;
    time_t shm_dtime;
    time_t dhm_ctime;
};

void *shmat(int shmid, const void *shmaddr, int shmflg);
int shmctl(int shmid, int cmd, struct shmid_ds *buf);
int shmdt(const void *shmaddr);
int shmget(key_t key, size_t length, int shmflg);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SYS_SHM_H */
