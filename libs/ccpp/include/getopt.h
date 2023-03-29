#pragma once

#include <ccpp/bits/config.h>

__CCPP_BEGIN_DECLARATIONS

#define no_argument       0
#define required_argument 1
#define optional_argument 2

struct option {
    char const* name;
    int has_arg;
    int* flag;
    int val;
};

int getopt_long(int __argc, char* const __argv[], char const* __optstring, struct option const* __longopts,
                int* __longindex);
int getopt_long_only(int __argc, char* const __argv[], char const* __optstring, struct option const* __longopts,
                     int* __longindex);

__CCPP_END_DECLARATIONS
