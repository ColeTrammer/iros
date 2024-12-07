#pragma once

#include "di/meta/language.h"

namespace di::util {
namespace detail {
    struct ToAddressFunction {
        template<typename T>
        requires(!concepts::LanguageFunction<T>)
        constexpr auto operator()(T* pointer) const -> T* {
            return pointer;
        }

        template<typename T>
        requires(requires(T const& pointer) { pointer.operator->(); })
        constexpr auto operator()(T const& pointer) const {
            return (*this)(pointer.operator->());
        }
    };
}

constexpr inline auto to_address = detail::ToAddressFunction {};
}

namespace di {
using util::to_address;
}
