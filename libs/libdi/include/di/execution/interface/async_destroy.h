#pragma once

#include <di/execution/concepts/sender_of.h>
#include <di/function/tag_invoke.h>
#include <di/types/prelude.h>

namespace di::execution {
namespace async_destroy_ns {
    struct InPlaceFunction {
        template<typename T, typename... Args>
        concepts::SenderOf<types::NoEnv> auto operator()(InPlaceType<T>, T& value) const
        requires(requires { function::tag_invoke(*this, in_place_type<T>, value); })
        {
            return function::tag_invoke(*this, in_place_type<T>, value);
        }
    };
}

constexpr inline auto async_destroy_in_place = async_destroy_ns::InPlaceFunction {};

namespace async_destroy_ns {
    template<typename T>
    struct Function {
        auto operator()(T& value) const
        requires(requires { async_destroy_in_place(in_place_type<T>, value); })
        {
            return async_destroy_in_place(in_place_type<T>, value);
        }
    };
}

template<typename T>
constexpr inline auto async_destroy = async_destroy_ns::Function<T> {};
}