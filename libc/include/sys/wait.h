#ifndef _SYS_WAIT_H
#define _SYS_WAIT_H 1

#include <sys/types.h>

#define WCONTINUED 1
#define WNOHANG 2
#define WUNTRACED 4

#define WEXITSTATUS(status) (((status) & 0xFF00) >> 8)
#define WTERMSIG(status) ((status) & 0x7F)

#define WIFCONTINUED(status) (status == 0xFFFF)
#define WIFEXITED(status) (WTERMSIG(status) == 0)
#define WIFSIGNALED(status) (WTERMSIG(status) != 0)
#define WIFSTOPPED(status) ((status) & 0x80)
#define WSTOPSIG(status) (WEXITSTATUS(status))

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

pid_t waitpid(pid_t pid, int *wstatus, int options);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SYS_WAIT_H */