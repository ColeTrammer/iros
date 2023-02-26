#include <dius/system/prelude.h>

static char const message[] = "Hello, World\n";

int main() {
    for (unsigned int i = 0; i < 2; i++) {
        (void) dius::system::system_call<i32>(dius::system::Number::debug_print, message, sizeof(message));
    }
    return 0;
}
