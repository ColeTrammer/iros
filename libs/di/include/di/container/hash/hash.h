#pragma once

#include <di/container/hash/hash_write.h>

namespace di::container {
namespace detail {
    struct HashFunction {
        constexpr auto operator()(concepts::Hashable auto const& value) const -> u64 {
            auto hasher = DefaultHasher {};
            container::hash_write(hasher, value);
            return hasher.finish();
        }

        constexpr auto operator()(concepts::Hasher auto& hasher, concepts::Hashable auto const& value) const -> u64 {
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
