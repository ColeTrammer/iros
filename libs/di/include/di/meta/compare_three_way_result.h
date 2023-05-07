#pragma once

#include <di/concepts/language_void.h>
#include <di/concepts/three_way_comparable.h>
#include <di/concepts/three_way_comparable_with.h>
#include <di/meta/list/type.h>
#include <di/meta/remove_reference.h>
#include <di/meta/type_constant.h>
#include <di/util/declval.h>

namespace di::meta {
namespace detail {
    template<typename T, typename U>
    struct CompareThreeWayResultHelper : meta::TypeConstant<void> {};

    template<typename T, typename U>
    requires(concepts::LanguageVoid<meta::Type<concepts::detail::DefinitelyThreeWayComparableWith<T, U>>> &&
             requires {
                 {
                     util::declval<meta::RemoveReference<T> const&>() <=>
                         util::declval<meta::RemoveReference<U> const&>()
                 };
             })
    struct CompareThreeWayResultHelper<T, U>
        : meta::TypeConstant<decltype(util::declval<meta::RemoveReference<T> const&>() <=>
                                      util::declval<meta::RemoveReference<U> const&>())> {};

    template<typename T, typename U>
    requires(!concepts::LanguageVoid<meta::Type<concepts::detail::DefinitelyThreeWayComparableWith<T, U>>>)
    struct CompareThreeWayResultHelper<T, U>
        : meta::TypeConstant<meta::Type<concepts::detail::DefinitelyThreeWayComparableWith<T, U>>> {};
}

template<typename T, typename U = T>
using CompareThreeWayResult = meta::Type<detail::CompareThreeWayResultHelper<T, U>>;
}
