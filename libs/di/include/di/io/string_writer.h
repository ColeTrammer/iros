#pragma once

#include <di/container/string/mutable_string.h>
#include <di/container/string/string.h>
#include <di/container/string/utf8_encoding.h>
#include <di/meta/core.h>
#include <di/meta/vocab.h>
#include <di/util/declval.h>
#include <di/vocab/error/prelude.h>

namespace di::io {
template<concepts::detail::MutableString T = container::String>
requires(concepts::SameAs<meta::Encoding<T>, container::string::Utf8Encoding>)
class StringWriter {
public:
    constexpr auto write_some(vocab::Span<byte const> data)
        -> meta::LikeExpected<decltype(util::declval<T&>().push_back(c32(0))), usize> {
        for (auto byte : data) {
            m_output.push_back(c32(byte));
        }
        return data.size();
    }

    constexpr vocab::Result<void> flush() { return {}; }

    constexpr T output() && { return util::move(m_output); }

private:
    T m_output;
};
}

namespace di {
using io::StringWriter;
}
