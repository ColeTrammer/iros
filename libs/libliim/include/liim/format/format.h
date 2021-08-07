#pragma once

#include <liim/format/base_formatters.h>
#include <liim/format/format_args_storage.h>
#include <liim/format/format_context.h>

namespace LIIM::Format {
String vformat(StringView format_string, FormatArgs format_args) {
    auto context = FormatContext {};
    for (;;) {
        auto left_brace = format_string.index_of('{');
        if (!left_brace) {
            context.put(format_string);
            return context.take_accumulator();
        }
        context.put(format_string.first(*left_brace));
        auto right_brace = format_string.substring(*left_brace).index_of('}');
        assert(right_brace);

        auto format_specifier = format_string.substring(*left_brace + 1, *right_brace - 1);
        (void) format_specifier;

        auto arg = format_args.next_arg();
        assert(arg);
        arg->do_format(context);

        format_string = format_string.substring(*left_brace + *right_brace + 1);
    }
}

template<typename... Args>
String format(StringView format_string, Args... args) {
    return vformat(format_string, make_format_args(args...));
}
}
