#include <dius/prelude.h>

namespace dius::system {
void exit_process(int code) {
    (void) dius::system::system_call<i32>(dius::system::Number::exit_task, code);
    di::unreachable();
}
}
