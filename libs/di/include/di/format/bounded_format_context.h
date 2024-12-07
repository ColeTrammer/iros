#pragma once

#include "di/container/string/string_impl.h"
#include "di/container/vector/prelude.h"
#include "di/util/move.h"

namespace di::format {
template<concepts::Encoding Enc, typename SizeConstant>
class BoundedFormatContext {
private:
    using Str = container::string::StringImpl<Enc, container::StaticVector<meta::EncodingCodeUnit<Enc>, SizeConstant>>;

public:
    using Encoding = Enc;

    constexpr void output(char c) { (void) m_output.push_back(c); }

    constexpr auto output() && -> Str { return util::move(m_output); }
    constexpr auto output() const& -> Str const& { return m_output; }

    constexpr auto encoding() const -> Encoding { return m_output.encoding(); }

private:
    Str m_output;
};
}
