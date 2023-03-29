#pragma once

#include <ccpp/bits/config.h>

#include __CCPP_PLATFORM_PATH(signal.h)

__CCPP_BEGIN_DECLARATIONS

void (*signal(int __sig, void (*__handler)(int)))(int);

__CCPP_END_DECLARATIONS
