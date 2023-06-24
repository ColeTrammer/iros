#pragma once

#include <di/types/prelude.h>
#include <di/vocab/span/prelude.h>

namespace di::concepts {
template<typename T>
concept Hasher = requires(T& hasher, vocab::Span<byte const> data) {
    { hasher.write(data) } -> SameAs<void>;
    { hasher.finish() } -> SameAs<u64>;
};
}

namespace di {
using concepts::Hasher;
}
