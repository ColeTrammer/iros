#pragma once

#include <di/container/string/string_impl.h>
#include <di/util/move.h>

namespace di::format {
template<concepts::Encoding Enc>
class FormatContext {
private:
    using Str = container::string::StringImpl<Enc>;

public:
    using Encoding = Enc;

    constexpr void output(char c) { m_output.push_back(c); }

    constexpr Str output() && { return util::move(m_output); }

    constexpr Encoding encoding() const { return m_output.encoding(); }

private:
    Str m_output;
};
}
