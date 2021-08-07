#pragma once

#include <liim/format/format_arg.h>
#include <liim/span.h>

namespace LIIM::Format {
class FormatArgs {
public:
    explicit FormatArgs(Span<FormatArg> args) : m_args(args) {}

    FormatArg* next_arg() {
        if (m_next_index >= m_args.size()) {
            return {};
        }
        return &m_args[m_next_index++];
    }

private:
    Span<FormatArg> m_args;
    size_t m_next_index { 0 };
};
}
