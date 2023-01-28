#pragma once

#include <di/meta/false_type.h>
#include <di/meta/remove_cv.h>
#include <di/meta/true_type.h>

namespace di::concepts {
namespace detail {
    template<typename T>
    struct MemberPointer : meta::FalseType {};

    template<typename T, typename U>
    struct MemberPointer<T U::*> : meta::TrueType {};
}

template<typename T>
concept MemberPointer = detail::MemberPointer<meta::RemoveCV<T>>::value;
}
