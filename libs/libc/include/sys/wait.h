#ifndef _SYS_WAIT_H
#define _SYS_WAIT_H 1

#include <bits/id_t.h>
#include <bits/pid_t.h>
#include <signal.h>

#define WCONTINUED 1
#define WNOHANG    2
#define WUNTRACED  4

#define WEXITSTATUS(status) (((status) &0xFF00) >> 8)
#define WTERMSIG(status)    ((status) &0x7F)

#define WIFCONTINUED(status) (status == 0xFFFF)
#define WIFEXITED(status)    (WTERMSIG(status) == 0 && !WIFSTOPPED(status) && !WIFCONTINUED(status) && !WIFSTOPPED(status))
#define WIFSIGNALED(status)  (WTERMSIG(status) != 0 && !WIFCONTINUED(status) && !WIFSTOPPED(status))
#define WIFSTOPPED(status)   ((status) &0x80 && !WIFCONTINUED(status))
#define WSTOPSIG(status)     (WEXITSTATUS(status))

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

pid_t wait(int *wstatus);
pid_t waitpid(pid_t pid, int *wstatus, int options);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SYS_WAIT_H */
