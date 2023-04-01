#include <stdlib.h>

extern "C" char* getenv(char const*) {
    return nullptr;
}
