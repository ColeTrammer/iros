#ifndef _SYS_SEM_H
#define _SYS_SEM_H 1

#include <bits/pid_t.h>
#include <bits/size_t.h>
#include <bits/time_t.h>
#include <sys/ipc.h>

#define SEM_UNDO 8

#define GETNCNT 1
#define GETPID  2
#define GETVAL  3
#define GETALL  4
#define GETZCNT 5
#define SETVAL  6
#define SETALL  7

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct semid_ds {
    struct ipc_perm sem_perm;
    unsigned short sem_nsems;
    time_t sem_otime;
    time_t sem_ctime;
};

struct sembuf {
    unsigned short sem_num;
    short sem_op;
    short sem_flg;
};

int semctl(int semid, int semnum, int cmd, ...);
int semget(key_t key, int nsems, int semflg);
int semop(int semid, struct sembuf *sops, size_t nsops);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SYS_SEM_H */
