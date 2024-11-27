#include <ccpp/bits/getopt_implementation.h>

namespace ccpp {
// https://pubs.opengroup.org/onlinepubs/9699919799/functions/getopt.html
extern "C" auto getopt(int argc, char* const argv[], char const* optstring) -> int {
    return getopt_implementation(argc, argv, optstring, nullptr, nullptr, false);
}
}
