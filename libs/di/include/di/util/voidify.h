#pragma once

namespace di::util {
namespace detail {
    struct VoidifyFunction {
        template<typename T>
        constexpr void* operator()(T* pointer) const {
            return const_cast<void*>(static_cast<void const volatile*>(pointer));
        }
    };
}

constexpr inline auto voidify = detail::VoidifyFunction {};
}

namespace di {
using util::voidify;
}
