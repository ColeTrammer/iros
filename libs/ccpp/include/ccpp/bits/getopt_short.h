#pragma once

#include <ccpp/bits/config.h>

__CCPP_BEGIN_DECLARATIONS

extern char* optarg;

extern int optind;
extern int opterr;
extern int optopt;

int getopt(int __argc, char* const __argv[], char const* __envp);

__CCPP_END_DECLARATIONS
