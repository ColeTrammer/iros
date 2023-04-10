#include <iris/core/global_state.h>
#include <iris/core/print.h>

namespace iris {
void log_prologue(detail::DebugFormatContext& context, di::SourceLocation location) {
    auto& global_state = iris::global_state_in_boot();
    auto& processor = [&] -> Processor& {
        if (!global_state.current_processor_available) {
            return global_state.boot_processor;
        }
        // SAFETY: this function is called with the debug lock held.
        return current_processor_unsafe();
    }();

    auto* current_task = processor.scheduler().current_task_null_if_during_boot();
    auto task_id = current_task ? current_task->id().raw_value() : 0;
    auto processor_id = processor.id();
    auto kernel = current_task ? current_task->address_space().is_kernel() : true;

    auto function_name = di::TransparentStringView(
        location.function_name(), di::to_unsigned(di::distance(di::ZCString(location.function_name()))));

    (void) di::format::present_encoded_context<Encoding>(
        "{}{} {} {} {}{}: {}: "_sv, context, di::Styled('[', di::FormatEffect::Bold),
        di::Styled("CPU"_sv, di::FormatColor::Yellow | di::FormatEffect::Bold),
        di::Styled(processor_id, di::FormatColor::Blue | di::FormatEffect::Bold),
        kernel ? di::Styled("Task"_sv, di::FormatColor::Magenta | di::FormatEffect::Bold)
               : di::Styled("Task"_sv, di::FormatColor::Green | di::FormatEffect::Bold),
        di::Styled(task_id, di::FormatColor::Blue | di::FormatEffect::Bold), di::Styled(']', di::FormatEffect::Bold),
        di::Styled(function_name, di::FormatColor::Cyan));
}
}
