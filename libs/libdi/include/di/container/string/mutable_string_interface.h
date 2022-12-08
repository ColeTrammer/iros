#pragma once

#include <di/container/string/constant_string_interface.h>
#include <di/container/string/encoding.h>
#include <di/container/string/string_append.h>
#include <di/container/string/string_push_back.h>
#include <di/container/vector/vector_clear.h>

namespace di::container::string {
template<typename Self, concepts::Encoding Enc>
class MutableStringInterface : public ConstantStringInterface<Self, Enc> {
private:
    using Encoding = Enc;
    using CodeUnit = meta::EncodingCodeUnit<Enc>;
    using CodePoint = meta::EncodingCodePoint<Enc>;
    using Iterator = meta::EncodingIterator<Enc>;

    constexpr Self& self() { return static_cast<Self&>(*this); }
    constexpr Self const& self() const { return static_cast<Self const&>(*this); }

public:
    constexpr void clear() { return vector::clear(self()); }

    constexpr auto push_back(CodePoint code_point) { return string::push_back(self(), code_point); }

    template<concepts::ContainerCompatible<CodePoint> Con>
    requires(concepts::SameAs<meta::Encoding<Con>, Encoding>)
    constexpr Self& append(Con&& container) {
        string::append(self(), util::forward<Con>(container));
        return self();
    }
};
}
