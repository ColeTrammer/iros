#pragma once

#include <ccpp/bits/config.h>
#include <ccpp/bits/time_t.h>

#if defined(__CCPP_C11) || defined(__CCPP_POSIX_EXTENSIONS)
#include <ccpp/bits/timespec.h>
#endif

__CCPP_BEGIN_DECLARATIONS

double difftime(time_t __time_end, time_t __time_start);
time_t time(time_t* __time);

__CCPP_END_DECLARATIONS
