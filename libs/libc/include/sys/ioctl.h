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
#define TIOCSWINSZ 9
#define TGETNUM    10
#define TIOCNOTTY  11
#define TIOSCTTY   12
#define TCIOFFI    13
#define TCOOFFI    14
#define TCIONI     15
#define TCOONI     16
#define TIOCGSID   17

#define SGWIDTH  0x1000
#define SGHEIGHT 0x1001
#define SSCURSOR 0x1002
#define SECURSOR 0x1003
#define SDCURSOR 0x1004
#define SSRES    0x1005
#define SSWAPBUF 0x1006

#define FIONBIO  0x2000
#define FIOCLEX  0x2001
#define FIONCLEX 0x2001

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct winsize {
    unsigned short ws_row;
    unsigned short ws_col;
    unsigned short ws_xpixel;
    unsigned short ws_ypixel;
};

struct cursor_pos {
    unsigned short cp_row;
    unsigned short cp_col;
};

struct screen_res {
    unsigned short x;
    unsigned short y;
};

int ioctl(int fd, unsigned long request, ...);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SYS_IOCTL_H */
