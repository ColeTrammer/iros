#include <iris/core/global_state.h>
#include <iris/core/print.h>
#include <iris/hw/power.h>

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

namespace di::assert::detail {
static di::TransparentStringView zstring_to_string_view(char const* s) {
    return di::TransparentStringView(s, di::to_unsigned(di::distance(di::ZCString(s))));
}

void assert_fail(char const* source_text, char const* lhs_message, char const* rhs_message, util::SourceLocation loc) {
    auto source_text_view = zstring_to_string_view(source_text);

    iris::println("{}: {}"_sv, di::Styled("ASSERT"_sv, di::FormatColor::Red | di::FormatEffect::Bold),
                  source_text_view);

    iris::println("{}: {}(): {}:{}:{}"_sv, di::Styled("AT"_sv, di::FormatEffect::Bold),
                  zstring_to_string_view(loc.function_name()), zstring_to_string_view(loc.file_name()), loc.line(),
                  loc.column());
    if (lhs_message) {
        auto lhs_message_view = zstring_to_string_view(lhs_message);
        iris::println("{}: {}"_sv, di::Styled("LHS"_sv, di::FormatEffect::Bold), lhs_message_view);
    }
    if (rhs_message) {
        auto rhs_message_view = zstring_to_string_view(rhs_message);
        iris::println("{}: {}"_sv, di::Styled("RHS"_sv, di::FormatEffect::Bold), rhs_message_view);
    }

    iris::println("Assertion failed, shutting down..."_sv);
    iris::hard_shutdown(iris::ShutdownStatus::Error);
}
}
