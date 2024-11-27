#pragma once

#include <di/format/concepts/format_arg.h>
#include <di/vocab/span/prelude.h>

namespace di::format {
template<concepts::FormatArg Arg>
class FormatArgs {
public:
    constexpr FormatArgs(Span<Arg> args) : m_args(args) {}

    constexpr auto size() const -> size_t { return m_args.size(); }

    constexpr auto operator[](size_t index) const -> Arg { return m_args[index]; }

    constexpr void set_args(Span<Arg> args) { m_args = args; }

private:
    Span<Arg> m_args;
};
}
