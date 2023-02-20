#pragma once

#include <ccpp/bits/config.h>

__CCPP_BEGIN_DECLARATIONS

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

struct FILE;
typedef struct FILE* FILE;

extern FILE stdin;
extern FILE stdout;
extern FILE stderr;

__CCPP_END_DECLARATIONS