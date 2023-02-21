#pragma once

#include <di/function/tag_invoke.h>
#include <di/types/prelude.h>

namespace di::vocab {
namespace detail {
    struct IntrusivePtrIncrement {
        template<typename Tag, typename T>
        requires(concepts::TagInvocable<IntrusivePtrIncrement, InPlaceType<Tag>, T*>)
        constexpr void operator()(InPlaceType<Tag>, T* ptr) const {
            return function::tag_invoke(*this, in_place_type<Tag>, ptr);
        }
    };

    struct IntrusivePtrDecrement {
        template<typename Tag, typename T>
        requires(concepts::TagInvocable<IntrusivePtrDecrement, InPlaceType<Tag>, T*>)
        constexpr void operator()(InPlaceType<Tag>, T* ptr) const {
            return function::tag_invoke(*this, in_place_type<Tag>, ptr);
        }
    };
}

constexpr inline auto intrusive_ptr_increment = detail::IntrusivePtrIncrement {};
constexpr inline auto intrusive_ptr_decrement = detail::IntrusivePtrDecrement {};

namespace detail {
    template<typename T, typename Tag>
    concept IntrusivePtrValid = requires(T* pointer) {
                                    intrusive_ptr_increment(in_place_type<Tag>, pointer);
                                    intrusive_ptr_decrement(in_place_type<Tag>, pointer);
                                };
}
}
