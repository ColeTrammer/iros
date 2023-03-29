#pragma once

#include <ccpp/bits/config.h>

#ifdef __cplusplus
#if (__cplusplus <= 199711L)
#define NULL (0)
#else
#define NULL (nullptr)
#endif
#else
#define NULL ((void*) 0)
#endif
