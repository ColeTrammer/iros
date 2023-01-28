#ifndef _TERMIOS_H
#define _TERMIOS_H 1

#include <bits/pid_t.h>

#define NCCS 11

#define VEOF   0
#define VEOL   1
#define VERASE 2
#define VINTR  3
#define VKILL  4
#define VMIN   5
#define VQUIT  6
#define VSTART 7
#define VSTOP  8
#define VSUSP  9
#define VTIME  10

#define BRKINT 0000002
#define ICRNL  0000400
#define IGNBRK 0000200
#define IGNCR  0001000
#define IGNPAR 0010000
#define INLCR  0004000
#define INPCK  0000020
#define ISTRIP 0000040
#define IXON   0002000
#define IXOFF  0010000
#define IXANY  0020000
#define PARMRK 0040000

#define OPOST  0000001
#define ONLCR  0000002
#define OCRNL  0000004
#define ONOCR  0000010
#define ONLRET 0000020
#define OFDEL  0000040
#define OFILL  0000100

#define NLDLY 0000100
#define NL0   0000000
#define NL1   0000100

#define CRDLY 0000600
#define CR0   0000000
#define CR1   0000200
#define CR2   0000300
#define CR3   0000400

#define TABDLY 0003000
#define TAB0   0000000
#define TAB1   0001000
#define TAB2   0002000
#define TAB3   0003000

#define BSDLY 0004000
#define BS0   0000000
#define BS1   0004000

#define VTDLY 0010000
#define VT0   0000000
#define VT1   0010000

#define FFDLY 0020000
#define FF0   0000000
#define FF1   0020000

#define B0     0000000
#define B50    0000001
#define B75    0000002
#define B110   0000003
#define B134   0000004
#define B150   0000005
#define B200   0000006
#define B300   0000007
#define B600   0000010
#define B1200  0000011
#define B1800  0000012
#define B2400  0000013
#define B4800  0000014
#define B9600  0000015
#define B19200 0000016
#define B38400 0000017

#define CSIZE 0xFF
#define CS5   0x1F
#define CS6   0x3F
#define CS7   0x7F
#define CS8   0xFF

#define CSTOPB 0x0100
#define CREAD  0x0200
#define PARENB 0x0400
#define PARODD 0x0800
#define HUPCL  0x1000
#define CLOCAL 0x2000

#define ECHO   0000010
#define ECHOE  0000100
#define ECHOK  0000020
#define ECHONL 0000040
#define ICANON 0000002
#define IEXTEN 0100000
#define ISIG   0000001
#define NOFLSH 0002000
#define TOSTOP 0000400

#define TCSANOW   1
#define TCSADRAIN 2
#define TCSAFLUSH 3

#define TCIFLUSH  1
#define TCIOFLUSH 2
#define TCOFLUSH  3

#define TCIOFF 1
#define TCION  2
#define TCOOFF 0
#define TCOON  3

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef unsigned char cc_t;
typedef unsigned int speed_t;
typedef unsigned int tcflag_t;

struct termios {
    tcflag_t c_iflag;
    tcflag_t c_oflag;
    tcflag_t c_cflag;
    tcflag_t c_lflag;
    cc_t c_cc[NCCS];
};

int tcgetattr(int fd, struct termios *termios_p);
int tcsetattr(int fd, int option_commands, const struct termios *termios_p);

speed_t cfgetispeed(const struct termios *termios_p);
speed_t cfgetospeed(const struct termios *termios_p);
int cfsetispeed(struct termios *termios_p, speed_t speed);
int cfsetospeed(struct termios *termios_p, speed_t speed);

int tcdrain(int fd);
int tcflow(int fd, int how);
int tcflush(int fd, int how);

pid_t tcgetsid(int fd);
int tcsendbreak(int fd, int duration);

void cfmakeraw(struct termios *termios_p);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _TERMIOS_H */
