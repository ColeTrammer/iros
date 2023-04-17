#pragma once

#include <di/container/string/string_impl.h>
#include <di/container/vector/prelude.h>
#include <di/util/move.h>

namespace di::format {
template<concepts::Encoding Enc, typename SizeConstant>
class BoundedFormatContext {
private:
    using Str = container::string::StringImpl<Enc, container::StaticVector<meta::EncodingCodeUnit<Enc>, SizeConstant>>;

public:
    using Encoding = Enc;

    constexpr void output(char c) { (void) m_output.push_back(c); }

    constexpr Str output() && { return util::move(m_output); }
    constexpr Str const& output() const& { return m_output; }

    constexpr Encoding encoding() const { return m_output.encoding(); }

private:
    Str m_output;
};
}
