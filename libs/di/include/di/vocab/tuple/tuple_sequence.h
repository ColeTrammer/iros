#pragma once

#include <di/function/invoke.h>
#include <di/function/monad/monad_try.h>
#include <di/vocab/tuple/apply.h>
#include <di/vocab/tuple/tuple_like.h>

namespace di::vocab {
namespace detail {
    template<typename R>
    struct TupleSequenceFunction {
        template<typename F, concepts::TupleLike T>
        constexpr auto operator()(F&& function, T&& tuple) const -> R {
            return vocab::apply(
                [&](auto&&... args) {
                    return helper(function, util::forward<decltype(args)>(args)...);
                },
                util::forward<T>(tuple));
        }

    private:
        template<typename F>
        constexpr static auto helper(F&&) -> R {
            return R();
        }

        template<typename F, typename T, typename... Ts>
        constexpr static auto helper(F&& function, T&& value, Ts&&... values) -> R {
            DI_TRY(function::invoke(function, util::forward<T>(value)));
            return helper(function, util::forward<Ts>(values)...);
        }
    };
}

template<typename R>
constexpr inline auto tuple_sequence = detail::TupleSequenceFunction<R> {};
}

namespace di {
using vocab::tuple_sequence;
}
