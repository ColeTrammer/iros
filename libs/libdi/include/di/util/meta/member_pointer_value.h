#pragma once

#include <di/util/concepts/member_pointer.h>
#include <di/util/meta/type_constant.h>

namespace di::util::meta {
namespace detail {
    template<typename T>
    struct MemberPointerValueHelper {};

    template<typename Value, typename Class>
    struct MemberPointerValueHelper<Value Class::*> : TypeConstant<Value> {};
}

template<concepts::MemberPointer T>
using MemberPointerValue = detail::MemberPointerValueHelper<RemoveCV<T>>::Type;
}
