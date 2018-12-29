#ifndef _UNISTD_H
#define _UNISTD_H 1

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif /* cplusplus */

int execv(const char*, char* const[]);
int execve(const char*, char* const[], char* const[]);
int execvp(const char*, char* const[]);
pid_t fork();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _UNISTD_H */