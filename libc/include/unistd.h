#ifndef _UNISTD_H
#define _UNISTD_H 1

#include <sys/types.h>
#include <stddef.h>
#include <stdbool.h>

typedef long intptr_t;

#ifdef __cplusplus
extern "C" {
#endif /* cplusplus */

void *sbrk(intptr_t increment);
bool sys_print(void *buffer, size_t n);

int execv(const char*, char* const[]);
int execve(const char*, char* const[], char* const[]);
int execvp(const char*, char* const[]);
pid_t fork();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _UNISTD_H */