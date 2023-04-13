#pragma once

#include <di/concepts/same_as.h>
#include <di/meta/bool_constant.h>
#include <di/meta/remove_cv.h>

namespace di::concepts {
namespace detail {
    template<typename From, typename To>
    struct QualificationConvertibleTo : meta::BoolConstant<SameAs<From, To>> {};

    template<typename From, typename To>
    struct QualificationConvertibleTo<From, To const> : meta::BoolConstant<SameAs<meta::RemoveConst<From>, To>> {};

    template<typename From, typename To>
    struct QualificationConvertibleTo<From, To volatile>
        : meta::BoolConstant<SameAs<meta::RemoveVolatile<From>, To>> {};

    template<typename From, typename To>
    struct QualificationConvertibleTo<From, To const volatile>
        : meta::BoolConstant<SameAs<meta::RemoveCV<From>, To>> {};

}

template<typename From, typename To>
concept QualificationConvertibleTo = detail::QualificationConvertibleTo<From, To>::value;
}
