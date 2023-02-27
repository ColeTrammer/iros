#include <dius/system/prelude.h>

static char const program[] = "/test_userspace";
static char const program2[] = "/test_read";

int main() {
    auto* x = new int;
    *x = 42;

    auto* y = new int;
    *y = 42;
    for (unsigned int i = 0; i < 3; i++) {
        auto tid = *dius::system::system_call<i32>(dius::system::Number::create_task);
        (void) dius::system::system_call<i32>(dius::system::Number::load_executable, tid, program, sizeof(program) - 1);
        (void) dius::system::system_call<i32>(dius::system::Number::start_task, tid);
    }

    {
        auto tid = *dius::system::system_call<i32>(dius::system::Number::create_task);
        (void) dius::system::system_call<i32>(dius::system::Number::load_executable, tid, program2,
                                              sizeof(program2) - 1);
        (void) dius::system::system_call<i32>(dius::system::Number::start_task, tid);
    }
    return 0;
}
