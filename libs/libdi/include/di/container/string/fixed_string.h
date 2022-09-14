#pragma once

#include <di/types/size_t.h>

namespace di::container {
template<typename CodeUnit, types::size_t size>
class FixedString {
public:
    CodeUnit m_data[size + 1];

    constexpr FixedString(CodeUnit const (&data)[size + 1]) {
        for (types::size_t i = 0; i < size; i++) {
            m_data[i] = data[i];
        }
        m_data[size] = '\0';
    }

    constexpr auto span() const { return m_data.span(); }

    template<types::size_t other_size>
    requires(size != other_size)
    constexpr friend bool operator==(FixedString const&, FixedString<CodeUnit, other_size> const&) {
        return false;
    }

    constexpr bool operator==(FixedString const&) const = default;
    constexpr auto operator<=>(FixedString const&) const = default;
};

template<typename CodeUnit, types::size_t size>
FixedString(CodeUnit const (&)[size]) -> FixedString<CodeUnit, size - 1>;
}
