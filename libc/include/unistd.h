#ifndef _UNISTD_H
#define _UNISTD_H 1

#include <sys/types.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#ifdef __cplusplus
extern "C" {
#endif /* cplusplus */

extern char **environ;

void *sbrk(intptr_t increment);
void _exit(int status) __attribute__((__noreturn__));
int execve(const char *file, char *const argv[], char *const envp[]);
pid_t getpid();
uid_t getuid();
int setpgid(pid_t pid, pid_t pgid);
char *getcwd(char *buf, size_t size);
int chdir(const char *path);
off_t lseek(int fd, off_t offset, int whence);

ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void * buf, size_t count);
int close(int fd);

int execv(const char *path, char *const[]);
int execvp(const char *file, char *const argv[]);
int execvpe(const char *file, char *const argv[], char *const envp[]);
pid_t fork();

int isatty(int fd);
int tcsetpgrp(int fd, pid_t pgid);
int ftruncate(int fd, off_t length);
int dup2(int oldfd, int newfd);
int pipe(int pipefd[2]);
int unlink(const char *pathname);
int rmdir(const char *pathname);

char *getlogin(void);
int getlogin_r(char *buf, size_t bufsize);

int getopt(int argc, char *const argv[], const char *optstring);

extern char *optarg;
extern int optind, opterr, optopt;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _UNISTD_H */