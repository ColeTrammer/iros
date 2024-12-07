#pragma once

#include "di/meta/core.h"
#include "di/meta/function.h"
#include "di/meta/language.h"
#include "di/types/integers.h"
#include "di/vocab/expected/expected_forward_declaration.h"
#include "di/vocab/optional/optional_forward_declaration.h"
#include "di/vocab/span/span_forward_declaration.h"
#include "di/vocab/tuple/tuple_forward_declaration.h"

namespace di::vocab {
template<typename T, usize size>
struct Array;
}

namespace di::concepts {
namespace detail {
    template<typename T>
    constexpr inline bool array_helper = false;

    template<typename T, usize size>
    constexpr inline bool array_helper<vocab::Array<T, size>> = true;
}

template<typename T>
concept Array = detail::array_helper<meta::RemoveCVRef<T>>;

template<typename T>
concept Expected = InstanceOf<meta::RemoveCVRef<T>, vocab::Expected>;
}

namespace di::meta {
template<concepts::Expected T>
using ExpectedValue = meta::RemoveCVRef<T>::Value;

template<concepts::Expected T>
using ExpectedError = meta::RemoveCVRef<T>::Error;

template<typename T>
constexpr inline auto ExpectedRank = 0ZU;

template<concepts::Expected T>
constexpr inline auto ExpectedRank<T> = 1 + ExpectedRank<ExpectedValue<T>>;

namespace detail {
    template<typename T, typename U>
    struct LikeExpectedHelper : TypeConstant<U> {};

    template<typename X, typename E, typename U>
    requires(!concepts::LanguageVoid<E>)
    struct LikeExpectedHelper<vocab::Expected<X, E>, U> : TypeConstant<vocab::Expected<U, E>> {};

    template<typename X, typename E, typename U>
    requires(concepts::LanguageVoid<E>)
    struct LikeExpectedHelper<vocab::Expected<X, E>, U> : TypeConstant<U> {};
}

template<typename T, typename U>
using LikeExpected = Type<detail::LikeExpectedHelper<T, U>>;

template<typename T>
using UnwrapExpected = Invoke<Conditional<concepts::Expected<T>, meta::Quote<ExpectedValue>, meta::TypeConstant<T>>, T>;
}

namespace di::concepts {
template<typename Exp, typename T>
concept ExpectedOf = Expected<Exp> && SameAs<meta::ExpectedValue<Exp>, T>;

template<typename Exp, typename T>
concept ExpectedError = Expected<Exp> && SameAs<meta::ExpectedError<Exp>, T>;

template<typename Fal, typename T>
concept MaybeFallible = SameAs<Fal, T> || ExpectedOf<Fal, T>;

template<typename T>
concept Optional = InstanceOf<meta::RemoveCVRef<T>, vocab::Optional>;
}

namespace di::meta {
template<concepts::Optional T>
using OptionalValue = meta::RemoveCVRef<T>::Value;

template<typename T>
constexpr inline auto OptionalRank = 0ZU;

template<concepts::Optional T>
constexpr inline auto OptionalRank<T> = 1 + OptionalRank<OptionalValue<T>>;
}

namespace di::concepts {
template<typename Opt, typename T>
concept OptionalOf = Optional<Opt> && SameAs<T, meta::OptionalValue<Opt>>;
}

namespace di::util {
template<typename T>
class ReferenceWrapper;
}

namespace di::concepts {
template<typename T>
concept ReferenceWrapper = InstanceOf<meta::RemoveCV<T>, util::ReferenceWrapper>;
}

namespace di::meta {
namespace detail {
    template<typename T>
    struct WrapReference : TypeConstant<T> {};

    template<concepts::LValueReference T>
    struct WrapReference<T> : TypeConstant<util::ReferenceWrapper<RemoveReference<T>>> {};
}

template<typename T>
using WrapReference = detail::WrapReference<T>::Type;

namespace detail {
    template<typename T>
    struct UnwrapReferenceHelper : TypeConstant<T> {};

    template<concepts::ReferenceWrapper T>
    struct UnwrapReferenceHelper<T> : TypeConstant<typename T::Value&> {};
}

template<typename T>
using UnwrapReference = detail::UnwrapReferenceHelper<T>::Type;

template<typename T>
using UnwrapRefDecay = UnwrapReference<Decay<T>>;

template<typename T>
using UnwrapRefRValue = RemoveRValueReference<UnwrapReference<T>>;
}

namespace di::concepts {
namespace detail {
    template<typename T>
    constexpr inline bool span_helper = false;

    template<typename T, usize extent>
    constexpr inline bool span_helper<vocab::Span<T, extent>> = true;
}

template<typename T>
concept Span = detail::span_helper<meta::RemoveCVRef<T>>;

template<typename T>
concept Tuple = InstanceOf<meta::RemoveCVRef<T>, vocab::Tuple>;

template<typename T>
concept Unexpected = InstanceOf<meta::RemoveCVRef<T>, vocab::Unexpected>;
}
