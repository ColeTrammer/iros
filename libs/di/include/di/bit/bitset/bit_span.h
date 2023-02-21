#pragma once

#include <di/types/prelude.h>

namespace di::bit {
template<size_t bit_offset, size_t extent>
class BitSpan {
public:
    constexpr explicit BitSpan(u8* data) : m_data(data) {}

private:
    u8* m_data { nullptr };
};
}
