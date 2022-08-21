#pragma once

#include <di/util/concepts/member_pointer.h>
#include <di/util/meta/remove_cv.h>
#include <di/util/meta/type_constant.h>

namespace di::util::meta {
namespace detail {
    template<typename T>
    struct MemberPointerClassHelper {};

    template<typename Value, typename Class>
    struct MemberPointerClassHelper<Value Class::*> : TypeConstant<Class> {};
}

template<concepts::MemberPointer T>
using MemberPointerClass = detail::MemberPointerClassHelper<RemoveCV<T>>::Type;
}
