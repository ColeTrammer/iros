#pragma once

#include <di/execution/concepts/sender_of.h>
#include <di/execution/receiver/set_value.h>
#include <di/function/tag_invoke.h>
#include <di/types/prelude.h>

namespace di::execution {
namespace async_create_ns {
    struct InPlaceFunction {
        template<typename T, typename... Args>
        concepts::SenderOf<SetValue(T)> auto operator()(InPlaceType<T>, Args&&... args) const
        requires(requires { function::tag_invoke(*this, in_place_type<T>, util::forward<Args>(args)...); })
        {
            return function::tag_invoke(*this, in_place_type<T>, util::forward<Args>(args)...);
        }
    };
}

constexpr inline auto async_create_in_place = async_create_ns::InPlaceFunction {};

namespace async_create_ns {
    template<typename T>
    struct Function {
        template<typename... Args>
        auto operator()(Args&&... args) const
        requires(requires { execution::async_create_in_place(in_place_type<T>, util::forward<Args>(args)...); })
        {
            return execution::async_create_in_place(in_place_type<T>, util::forward<Args>(args)...);
        }
    };
}

template<typename T>
constexpr inline auto async_create = async_create_ns::Function<T> {};
}
