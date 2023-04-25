#pragma once

#include <di/function/tag_invoke.h>
#include <di/meta/list/as_list.h>
#include <di/meta/remove_cvref.h>
#include <di/types/prelude.h>
#include <di/vocab/tuple/tuple_like.h>

namespace di::reflection {
namespace detail {
    struct ReflectFunction {
        template<typename T>
        requires(concepts::TagInvocable<ReflectFunction, InPlaceType<T>>)
        constexpr decltype(auto) operator()(InPlaceType<T>) const {
            static_assert(concepts::TupleLike<meta::TagInvokeResult<ReflectFunction, InPlaceType<T>>>,
                          "Reflect function must return a tuple of fields");
            return function::tag_invoke(*this, in_place_type<T>);
        }

        template<typename T, typename U = meta::RemoveCVRef<T>>
        requires(concepts::TagInvocable<ReflectFunction, InPlaceType<U>>)
        constexpr decltype(auto) operator()(T&&) const {
            return (*this)(in_place_type<U>);
        }
    };
}

constexpr inline auto reflect = detail::ReflectFunction {};
}

namespace di::concepts {
template<typename T>
concept Reflectable = requires {
    { reflection::reflect(in_place_type<T>) };
};
}

namespace di::meta {
template<concepts::Reflectable T>
using Reflect = meta::AsList<decltype(reflection::reflect(in_place_type<T>))>;
}
