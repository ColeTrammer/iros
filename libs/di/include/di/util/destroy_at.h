#pragma once

#include "di/meta/language.h"
#include "di/meta/operations.h"
#include "di/meta/trivial.h"
#include "di/util/addressof.h"

namespace di::util {
namespace detail {
    struct DestroyAtFunction {
        template<concepts::Destructible T>
        constexpr void operator()(T* pointer) const {
            if constexpr (concepts::LanguageArray<T>) {
                for (auto& item : *pointer) {
                    (*this)(util::addressof(item));
                }
            } else if constexpr (!concepts::TriviallyDestructible<T>) {
                pointer->~T();
            }
        }
    };
}

constexpr inline auto destroy_at = detail::DestroyAtFunction {};
}

namespace di {
using util::destroy_at;
}
