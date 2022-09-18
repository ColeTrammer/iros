#pragma once

#include <di/types/size_t.h>

namespace di::container {
template<typename CodeUnit, types::size_t count>
class FixedString {
public:
    CodeUnit m_data[count + 1];

    constexpr FixedString(CodeUnit const (&data)[count + 1]) {
        for (types::size_t i = 0; i < count; i++) {
            m_data[i] = data[i];
        }
        m_data[count] = '\0';
    }

    constexpr CodeUnit const* data() const { return m_data; }
    constexpr auto size() const { return count; }

    template<types::size_t other_size>
    requires(count != other_size)
    constexpr friend bool operator==(FixedString const&, FixedString<CodeUnit, other_size> const&) {
        return false;
    }

    constexpr bool operator==(FixedString const&) const = default;
    constexpr auto operator<=>(FixedString const&) const = default;
};

template<typename CodeUnit, types::size_t size>
FixedString(CodeUnit const (&)[size]) -> FixedString<CodeUnit, size - 1>;
}
