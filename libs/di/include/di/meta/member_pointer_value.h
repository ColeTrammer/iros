#pragma once

#include <di/concepts/member_pointer.h>
#include <di/meta/core.h>

namespace di::meta {
namespace detail {
    template<typename T>
    struct MemberPointerValueHelper {};

    template<typename Value, typename Class>
    struct MemberPointerValueHelper<Value Class::*> : TypeConstant<Value> {};
}

template<concepts::MemberPointer T>
using MemberPointerValue = detail::MemberPointerValueHelper<RemoveCV<T>>::Type;
}
