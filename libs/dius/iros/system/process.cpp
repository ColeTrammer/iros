#include <dius/prelude.h>

namespace dius::system {
di::Result<void> Process::swawn_and_wait() && {
    auto arguments_as_view = m_arguments | di::transform([](di::TransparentString const& arg) {
                                 return arg.view();
                             }) |
                             di::to<di::Vector>();

    auto tid = TRY(system_call<i32>(Number::create_task));
    TRY(system_call<i32>(Number::set_task_arguments, tid, arguments_as_view.data(), arguments_as_view.size(), nullptr,
                         0));
    TRY(system_call<i32>(Number::load_executable, tid, m_arguments[0].data(), m_arguments[0].size()));
    TRY(system_call<i32>(Number::start_task_and_block, tid));

    return {};
}

void exit_thread() {
    (void) system_call<i32>(Number::exit_task);
    di::unreachable();
}

void exit_process(int code) {
    // FIXME: exit the entire process instead of just the current task.
    (void) system_call<i32>(Number::exit_task, code);
    di::unreachable();
}
}
