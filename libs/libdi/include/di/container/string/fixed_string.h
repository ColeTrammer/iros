#pragma once

#include <di/types/size_t.h>

namespace di::container {
template<types::size_t count>
class FixedString {
public:
    char m_data[count + 1];

    constexpr FixedString(char const (&data)[count + 1]) {
        for (types::size_t i = 0; i < count; i++) {
            m_data[i] = data[i];
        }
        m_data[count] = '\0';
    }

    constexpr char const* data() const { return m_data; }
    constexpr static auto size() { return count; }

    template<types::size_t other_size>
    requires(count != other_size)
    constexpr friend bool operator==(FixedString const&, FixedString<other_size> const&) {
        return false;
    }

    bool operator==(FixedString const&) const = default;
    auto operator<=>(FixedString const&) const = default;
};

template<types::size_t size>
FixedString(char const (&)[size]) -> FixedString<size - 1>;
}
