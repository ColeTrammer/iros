#pragma once

#include <getopt.h>

namespace ccpp {
int getopt_implementation(int argc, char* const argv[], char const* optstring, struct option const* longopts,
                          int* longindex, bool long_only);
}
