#include <di/assert/prelude.h>
#include <di/util/prelude.h>

extern "C" {
uptr __stack_chk_guard = 0x595e9fbd94fda766;
}

extern "C" [[noreturn]] void __stack_chk_fail(void) {
    DI_ASSERT(false);
    di::unreachable();
}
