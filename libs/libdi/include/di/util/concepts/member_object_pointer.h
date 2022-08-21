#pragma once

#include <di/util/concepts/member_function_pointer.h>
#include <di/util/concepts/member_pointer.h>

namespace di::util::concepts {
template<typename T>
concept MemberObjectPointer = MemberPointer<T> && !MemberFunctionPointer<T>;
}
