#pragma once

#include <liim/format/format.h>
#include <stdio.h>

namespace LIIM {
template<typename... Args>
void debug_log(StringView format_string, const Args&... args) {
    auto result = vformat(format_string, Format::make_format_args(args...));
    fprintf(stderr, "%s\n", result.string());
}

template<typename... Args>
void error_log(StringView format_string, const Args&... args) {
    auto result = vformat(format_string, Format::make_format_args(args...));
    fprintf(stderr, "%s\n", result.string());
}

template<typename... Args>
void out_log(StringView format_string, const Args&... args) {
    auto result = vformat(format_string, Format::make_format_args(args...));
    puts(result.string());
}
}

using LIIM::debug_log;
using LIIM::error_log;
using LIIM::out_log;
using LIIM::Format::format;
using LIIM::Format::make_format_args;
using LIIM::Format::vformat;
