#ifndef _UNISTD_H
#define _UNISTD_H 1

#include <sys/types.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* cplusplus */

void *sbrk(intptr_t increment);
bool sys_print(void *buffer, size_t n);

ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void * buf, size_t count);
int close(int fd);

int execv(const char*, char* const[]);
int execve(const char*, char* const[], char* const[]);
int execvp(const char*, char* const[]);
pid_t fork();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _UNISTD_H */