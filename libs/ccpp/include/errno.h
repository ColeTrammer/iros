#pragma once

#include <ccpp/bits/config.h>

#include __CCPP_PLATFORM_PATH(errno.h)

__CCPP_BEGIN_DECLARATIONS

extern __thread int errno;

#define errno errno

__CCPP_END_DECLARATIONS
