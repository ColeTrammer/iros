#ifndef _TERMIOS_H
#define _TERMIOS_H 1

#define NCCS 11

#define VEOF 0
#define VEOL 1
#define VERASE 2
#define VINTR 3
#define VKILL 4
#define VMIN 5
#define VQUIT 6
#define VSTART 7
#define VSTOP 8
#define VSUSP 9
#define VTIME 10

#define ECHO 0000010

#define TCSANOW 1
#define TCSADRAIN 2
#define TCSAFLUSH 3

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef long cc_t;
typedef long speed_t;
typedef long tcflag_t;

struct termios {
    tcflag_t c_iflag;
    tcflag_t c_oflag;
    tcflag_t c_cflag;
    tcflag_t c_lflag;
    cc_t c_cc[NCCS];
};

int tcgetattr(int fd, struct termios *termios_p);
int tcsetattr(int fd, int option_commands, const struct termios *termios_p);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _TERMIOS_H */