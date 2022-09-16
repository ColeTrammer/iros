#pragma once

#include <di/container/string/constant_string.h>
#include <di/container/string/encoding.h>
#include <di/container/string/string_begin.h>
#include <di/container/string/string_data.h>
#include <di/container/string/string_empty.h>
#include <di/container/string/string_end.h>
#include <di/container/string/string_size.h>
#include <di/container/string/string_size_code_units.h>

namespace di::container::string {
template<concepts::Encoding Enc>
class StringViewImpl;

template<typename Self, concepts::Encoding Enc>
class ConstantStringInterface {
private:
    using CodeUnit = meta::EncodingCodeUnit<Enc>;
    using CodePoint = meta::EncodingCodePoint<Enc>;
    using Iterator = meta::EncodingIterator<Enc>;

    constexpr Self const& self() const { return static_cast<Self const&>(*this); }

public:
    constexpr size_t size() const
    requires(encoding::Contiguous<Enc>)
    {
        return string::size(self());
    }

    constexpr size_t size_code_units() const { return string::size_code_units(self()); }
    constexpr bool empty() const { return string::empty(self()); }

    constexpr auto data() const { return string::data(self()); }

    constexpr auto begin() const { return string::begin(self()); }
    constexpr auto end() const { return string::end(self()); }

    constexpr auto view() const { return StringViewImpl<Enc>(self()); }

private:
    template<concepts::detail::ConstantString Other>
    requires(concepts::SameAs<Enc, meta::Encoding<Other>>)
    constexpr friend bool operator==(Self const& a, Other const& b) {
        auto at = string::begin(a);
        auto bt = string::begin(b);
        auto ae = string::end(a);
        auto be = string::end(b);
        for (; at != ae && bt != be; ++at, ++bt) {
            if (*at != *bt) {
                return false;
            }
        }
        return at == ae && bt == be;
    }
};
}
