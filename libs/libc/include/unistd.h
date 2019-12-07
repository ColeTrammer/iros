#ifndef _UNISTD_H
#define _UNISTD_H 1

#include <bits/gid_t.h>
#include <bits/null.h>
#include <bits/off_t.h>
#include <bits/pid_t.h>
#include <bits/size_t.h>
#include <bits/ssize_t.h>
#include <bits/uid_t.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define _POSIX_VERSION 200809L
#define _XOPEN_VERSION 700

#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#define F_OK 0
#define W_OK 1
#define R_OK 2
#define X_OK 4

#ifdef __cplusplus
extern "C" {
#endif /* cplusplus */

extern char **environ;

void *sbrk(intptr_t increment);
void _exit(int status) __attribute__((__noreturn__));
pid_t fork();
pid_t getpid();
uid_t getuid();
gid_t getgid();
uid_t geteuid();
gid_t getegid();
pid_t getppid();
int setuid(uid_t uid);
int setgid(gid_t gid);
int seteuid(uid_t uid);
int setegid(gid_t gid);
int setpgid(pid_t pid, pid_t pgid);
pid_t getpgid(pid_t pid);
pid_t getpgrp(void);
char *getcwd(char *buf, size_t size);
int chdir(const char *path);
off_t lseek(int fd, off_t offset, int whence);
unsigned int sleep(unsigned int seconds);
int chown(const char *pathname, uid_t owner, gid_t group);
int pause(void);

ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
int close(int fd);

int execl(const char *path, const char *arg, ...);
int execle(const char *path, const char *arg, ...);
int execlp(const char *name, const char *arg, ...);
int execv(const char *path, char *const args[]);
int execve(const char *path, char *const argv[], char *const envp[]);
int execvp(const char *file, char *const argv[]);
int execvpe(const char *file, char *const argv[], char *const envp[]);

int isatty(int fd);
int tcgetpgrp(int fd);
int tcsetpgrp(int fd, pid_t pgid);
int ftruncate(int fd, off_t length);
int dup(int oldfd);
int dup2(int oldfd, int newfd);
int pipe(int pipefd[2]);
int unlink(const char *pathname);
int rmdir(const char *pathname);
int access(const char *pathname, int mode);

unsigned int alarm(unsigned int seconds);
char *ttyname(int fd);
int ttyname_r(int fd, char *buf, size_t buflen);

char *getlogin(void);
int getlogin_r(char *buf, size_t bufsize);

int getopt(int argc, char *const argv[], const char *optstring);

extern char *optarg;
extern int optind, opterr, optopt;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _UNISTD_H */