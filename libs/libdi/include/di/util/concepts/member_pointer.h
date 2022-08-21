#pragma once

#include <di/util/meta/false_type.h>
#include <di/util/meta/remove_cv.h>
#include <di/util/meta/true_type.h>

namespace di::util::concepts {
namespace detail {
    template<typename T>
    struct MemberPointer : meta::FalseType {};

    template<typename T, typename U>
    struct MemberPointer<T U::*> : meta::TrueType {};
}

template<typename T>
concept MemberPointer = detail::MemberPointer<meta::RemoveCV<T>>::value;
}
