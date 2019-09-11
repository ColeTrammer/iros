#ifndef _FCNTL_H
#define _FCNTL_H 1

#include <sys/stat.h>
#include <unistd.h>

#define F_DUPFD         1
#define F_DUPFD_CLOEXEC 2
#define F_GETFD         3
#define F_SETFD         4
#define F_GETFL         5
#define F_SETFL         6
#define F_GETLK         7
#define F_SETLK         8
#define F_SETLKW        9
#define F_GETOWN        10
#define F_SETOWN        11

#define FD_CLOEXEC      1

#define F_RDLCK         0
#define F_UNLCK         1
#define F_WRLCK         2

/* Seek Constants */

#define O_CLOEXEC       1
#define O_CREAT         2
#define O_DIRECTORY     4
#define O_EXCL          8
#define O_NOCTTY        16
#define O_NOFOLLOW      32
#define O_TRUNC         64
#define O_TTY_INIT      0

#define O_APPEND        128
#define O_DSYNC         256
#define O_NONBLOCK      512
#define O_RSYNC         1024
#define O_SYNC          2048

#define O_ACCMODE       3

#define O_EXEC          4
#define O_RDONLY        0
#define O_RDWR          1
#define O_SEARCH        2
#define O_WRONLY        3

#ifdef __cplusplus
extern "C" { 
#endif /* __cplusplus */

int open(const char *pathname, int flags, mode_t mode);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _FCNTL_H */