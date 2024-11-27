#pragma once

namespace di::util {
namespace detail {
    struct VoidifyFunction {
        template<typename T>
        constexpr auto operator()(T* pointer) const -> void* {
            return const_cast<void*>(static_cast<void const volatile*>(pointer));
        }
    };
}

constexpr inline auto voidify = detail::VoidifyFunction {};
}

namespace di {
using util::voidify;
}
