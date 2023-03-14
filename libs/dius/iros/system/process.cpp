#include <dius/prelude.h>

namespace dius::system {
di::Result<void> Process::swawn_and_wait() && {
    // FIXME: pass arguments.

    auto tid = TRY(system_call<i32>(Number::create_task));
    TRY(system_call<i32>(Number::load_executable, tid, m_arguments[0].data(), m_arguments[0].size()));
    TRY(system_call<i32>(Number::start_task_and_block, tid));

    return {};
}

void exit_process(int code) {
    (void) system_call<i32>(Number::exit_task, code);
    di::unreachable();
}
}
