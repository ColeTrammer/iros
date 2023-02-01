#pragma once

#include <di/util/construct_at.h>
#include <di/util/std_new.h>
#include <di/util/voidify.h>

namespace di::util {
namespace detail {
    struct DefaultConstructAtFunction {
        template<typename T>
        requires(requires(void* pointer) { ::new (pointer) T; })
        constexpr T* operator()(T* location) const {
            // NOTE: this is not actually the same behavior, as the
            //       expression shown below leaves trivial types
            //       uninitialized. However, placement new is not
            //       usable in constexpr context, so we cannot
            //       call it here.
            if consteval {
                std::construct_at(location);
            } else {
                return ::new (util::voidify(location)) T;
            }
        }
    };
}

constexpr inline auto default_construct_at = detail::DefaultConstructAtFunction {};
}
