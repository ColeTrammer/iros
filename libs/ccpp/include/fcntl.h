#pragma once

#include <ccpp/bits/config.h>

__CCPP_BEGIN_DECLARATIONS
#define O_RDONLY 0x0001
#define O_WRONLY 0x0002
#define O_RDWR   0x0004
#define O_CREAT  0x0008
#define O_EXCL   0x0010
#define O_TRUNC  0x0020

int open(char const* __CCPP_RESTRICT __path, int __flags, ...);
__CCPP_END_DECLARATIONS
