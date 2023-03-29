#pragma once

#include <ccpp/bits/config.h>

__CCPP_BEGIN_DECLARATIONS

struct stat {};

int stat(char const* __CCPP_RESTRICT __path, struct stat* __CCPP_RESTRICT __info);

__CCPP_END_DECLARATIONS
