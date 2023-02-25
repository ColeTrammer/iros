#include <dius/system/prelude.h>

static char const program[] = "/test_userspace";

extern "C" [[noreturn]] void _start() {
    for (unsigned int i = 0; i < 3; i++) {
        auto tid = *dius::system::system_call<i32>(dius::system::Number::create_task);
        (void) dius::system::system_call<i32>(dius::system::Number::load_executable, tid, program, sizeof(program) - 1);
        (void) dius::system::system_call<i32>(dius::system::Number::start_task, tid);
    }
    (void) dius::system::system_call<i32>(dius::system::Number::exit_task);
    di::unreachable();
}
