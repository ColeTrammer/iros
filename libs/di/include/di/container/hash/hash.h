#pragma once

#include <di/container/hash/hash_write.h>

namespace di::container {
namespace detail {
    struct HashFunction {
        constexpr u64 operator()(concepts::Hashable auto const& value) const {
            auto hasher = DefaultHasher {};
            container::hash_write(hasher, value);
            return hasher.finish();
        }

        constexpr u64 operator()(concepts::Hasher auto& hasher, concepts::Hashable auto const& value) const {
            container::hash_write(hasher, value);
            return hasher.finish();
        }
    };
}

constexpr inline auto hash = detail::HashFunction {};
}

namespace di {
using container::hash;
}
