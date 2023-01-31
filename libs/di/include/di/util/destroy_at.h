#pragma once

#include <di/concepts/destructible.h>
#include <di/concepts/language_array.h>
#include <di/concepts/trivially_destructible.h>
#include <di/util/addressof.h>

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
