#include <dius/print.h>

extern "C" int main() {
    for (unsigned int i = 0; i < 2; i++) {
        dius::println("Hello, World!"_sv);
    }
    return 0;
}
