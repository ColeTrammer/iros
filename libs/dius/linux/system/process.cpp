#include <dius/prelude.h>

#ifndef DIUS_USE_RUNTIME
#include <stdlib.h>
#endif

namespace dius::system {
void exit_process(int code) {
#ifdef DIUS_USE_RUNTIME
    (void) dius::system::system_call<i32>(dius::system::Number::exit_group, code);
    di::unreachable();
#else
    exit(code);
#endif
}
}
