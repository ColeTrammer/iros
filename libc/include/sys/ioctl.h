#ifndef _SYS_IOCTL_H
#define _SYS_IOCTL_H 1

#define TCGETS  1
#define TCSETS  2
#define TCSETSW 3
#define TCSETSF 4

int ioctl(int fd, unsigned long request, ...);

#endif /* _SYS_IOCTL_H */