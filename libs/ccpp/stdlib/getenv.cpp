#include <stdlib.h>

extern "C" auto getenv(char const*) -> char* {
    return nullptr;
}
