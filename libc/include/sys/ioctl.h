#ifndef _SYS_IOCTL_H
#define _SYS_IOCTL_H 1

#define TCGETS     1
#define TCSETS     2
#define TCSETSW    3
#define TCSETSF    4
#define TIOCGWINSZ 5
#define TIOCSPGRP  6
#define TIOCGPGRP  7
#define TISATTY    8

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct winsize {
    unsigned short ws_row;
    unsigned short ws_col;
};

int ioctl(int fd, unsigned long request, ...);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SYS_IOCTL_H */