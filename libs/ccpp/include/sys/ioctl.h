#pragma once

#include <ccpp/bits/config.h>

__CCPP_BEGIN_DECLARATIONS

struct winsize {
    unsigned short ws_row;
    unsigned short ws_col;
    unsigned short ws_xpixel;
    unsigned short ws_ypixel;
};

#define TIOCGWINSZ 1

int ioctl(int __fd, unsigned long __request, ...);

__CCPP_END_DECLARATIONS
