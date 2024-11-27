#pragma once

#include <di/container/hash/hasher.h>
#include <di/util/exchange.h>

namespace di::container {
struct DefaultHasher {
    constexpr void write(vocab::Span<byte const> data) noexcept {
        // For now, this is a simple hash function. This is apparently used by Java.
        // See https://computinglife.wordpress.com/2008/11/20/why-do-hash-functions-use-prime-numbers/.
        for (auto const byte : data) {
            m_hash = (m_hash * 31u) + di::to_integer<u8>(byte);
        }
    }

    constexpr auto finish() noexcept -> u64 { return util::exchange(m_hash, 0); }

private:
    u64 m_hash { 0 };
};
}

namespace di {
using container::DefaultHasher;
}
