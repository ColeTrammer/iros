#ifndef _SYS_IPC_H
#define _SYS_IPC_H 1

#include <bits/gid_t.h>
#include <bits/key_t.h>
#include <bits/mode_t.h>
#include <bits/uid_t.h>

#define IPC_CREAT  2
#define IPC_EXCL   8
#define IPC_NOWAIT 512

#define IPC_PRIVATE 0

#define IPC_RMID 1
#define IPC_SET  2
#define IPC_STAT 3

#ifdef __cplusplus
extern "C"
#endif /* __cplusplus */

    struct ipc_perm {
    uid_t uid;
    gid_t gid;
    uid_t cuid;
    gid_t cgid;
    mode_t mode;
};

key_t ftok(const char *name, int id);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SYS_IPC_H */
