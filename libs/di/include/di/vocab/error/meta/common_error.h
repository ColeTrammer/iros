#pragma once

#include <di/meta/algorithm.h>
#include <di/meta/common.h>
#include <di/meta/core.h>
#include <di/meta/operations.h>
#include <di/util/declval.h>
#include <di/vocab/error/error.h>
#include <di/vocab/variant/variant.h>

namespace di::meta {
template<typename T, typename U>
struct CustomCommonError {};

namespace detail {
    template<typename T, typename U>
    concept HasCustomCommonError = requires { typename CustomCommonError<T, U>::Type; };

    template<typename... Types>
    struct CommonErrorHelper {};

    template<typename T>
    struct CommonErrorHelper<T> : CommonErrorHelper<T, T> {};

    template<typename... Ts, typename... Us>
    struct CommonErrorHelper<Variant<Ts...>, Variant<Us...>>
        : TypeConstant<AsTemplate<Variant, Unique<List<Ts..., Us...>>>> {};

    template<typename... Ts, typename U>
    struct CommonErrorHelper<Variant<Ts...>, U> : CommonErrorHelper<Variant<Ts...>, Variant<U>> {};

    template<typename T, typename... Us>
    struct CommonErrorHelper<T, Variant<Us...>> : CommonErrorHelper<Variant<Us...>, T> {};

    template<typename T, typename U>
    requires(HasCustomCommonError<T, U>)
    struct CommonErrorHelper<T, U> : CustomCommonError<T, U> {};

    template<typename T, typename U>
    requires(!HasCustomCommonError<T, U> && concepts::CommonWith<T, U>)
    struct CommonErrorHelper<T, U> : TypeConstant<CommonType<T, U>> {};

    template<typename T>
    concept ConvertibleToError = concepts::ConvertibleTo<T, Error>;

    template<typename T, typename U>
    concept CommonErrorIsError = ConvertibleToError<T> && ConvertibleToError<U>;

    template<typename T, typename U>
    requires(!HasCustomCommonError<T, U> && !concepts::CommonWith<T, U> && CommonErrorIsError<T, U>)
    struct CommonErrorHelper<T, U> : TypeConstant<Error> {};

    template<typename T, typename U>
    requires(!HasCustomCommonError<T, U> && !concepts::CommonWith<T, U> && !CommonErrorIsError<T, U>)
    struct CommonErrorHelper<T, U> : TypeConstant<Variant<T, U>> {};

    template<typename T, typename U, typename W, typename... Rest>
    requires(requires { typename CommonErrorHelper<T, U>::Type; })
    struct CommonErrorHelper<T, U, W, Rest...>
        : CommonErrorHelper<typename CommonErrorHelper<T, U>::Type, W, Rest...> {};
}

template<typename... Types>
requires(requires { typename detail::CommonErrorHelper<Types...>::Type; })
using CommonError = detail::CommonErrorHelper<Types...>::Type;
}

namespace di::concepts {
template<typename T, typename U>
concept CommonErrorWith = SameAs<meta::CommonError<T, U>, meta::CommonError<U, T>> && requires {
    static_cast<meta::CommonError<T, U>>(di::declval<T>());
    static_cast<meta::CommonError<T, U>>(di::declval<U>());
};
}
