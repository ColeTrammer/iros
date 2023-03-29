#pragma once

#include <ccpp/bits/config.h>
#include <ccpp/bits/time_t.h>

__CCPP_BEGIN_DECLARATIONS

double difftime(time_t __time_end, time_t __time_start);
time_t time(time_t* __time);

__CCPP_END_DECLARATIONS
