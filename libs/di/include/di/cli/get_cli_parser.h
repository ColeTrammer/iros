#pragma once

#include <di/function/tag_invoke.h>
#include <di/types/prelude.h>

namespace di::cli {
namespace detail {
    struct GetCliParserInPlaceFunction {
        template<typename T>
        requires(concepts::TagInvocable<GetCliParserInPlaceFunction, InPlaceType<T>> ||
                 requires { T::get_cli_parser(); })
        constexpr auto operator()(InPlaceType<T>) const {
            if constexpr (concepts::TagInvocable<GetCliParserInPlaceFunction, InPlaceType<T>>) {
                return function::tag_invoke(*this, in_place_type<T>);
            } else {
                return T::get_cli_parser();
            }
        }
    };
}

constexpr inline auto get_cli_parser_in_place = detail::GetCliParserInPlaceFunction {};

namespace detail {
    template<typename T>
    struct GetCliParserFunction {
        constexpr auto operator()() const { return get_cli_parser_in_place(in_place_type<T>); }
    };
}

template<typename T>
constexpr inline auto get_cli_parser = detail::GetCliParserFunction<T> {};
}