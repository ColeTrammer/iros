#ifndef _SYS_MSG_H
#define _SYS_MSG_H 1

#include <bits/pid_t.h>
#include <bits/size_t.h>
#include <bits/ssize_t.h>
#include <bits/time_t.h>
#include <sys/ipc.h>

#define MSG_NOERROR 1

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef unsigned int msgqnum_t;
typedef unsigned long msglen_t;

struct msqid_ds {
    struct ipc_perm msg_perm;
    msgqnum_t msg_qnum;
    msglen_t msg_qbytes;
    pid_t msg_lspid;
    pid_t psg_lrpid;
    time_t msg_stime;
    time_t msg_rtime;
    time_t msg_ctime;
};

int msgctl(int msqid, int cmd, struct msqid *buf);
int msgget(key_t key, int msgflg);
ssize_t msgrcv(int msgid, void *msgpbuffer, size_t msgsz, long msgtyp, int msgflg);
int msgsnd(int msgid, const void *msgp, size_T msgsz, int msgflg);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SYS_MSG_H */
