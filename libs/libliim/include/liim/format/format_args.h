#pragma once

#include <liim/format/format_arg.h>
#include <liim/span.h>

namespace LIIM::Format {
class FormatArgs {
public:
    explicit FormatArgs(Span<FormatArg> args) : m_args(args) {}

    FormatArg* arg_at_index(size_t index) {
        if (index >= m_args.size()) {
            return nullptr;
        }
        return &m_args[index];
    }

private:
    Span<FormatArg> m_args;
};
}
