#include <ccpp/bits/getopt_implementation.h>

namespace ccpp {
// https://www.gnu.org/software/libc/manual/html_node/Getopt-Long-Options.html
extern "C" int getopt_long_only(int argc, char* const* argv, char const* optstring, const struct option* longopts,
                                int* longindex) {
    return getopt_implementation(argc, argv, optstring, longopts, longindex, true);
}
}
