#pragma once

#include <di/concepts/member_function_pointer.h>
#include <di/concepts/member_pointer.h>

namespace di::concepts {
template<typename T>
concept MemberObjectPointer = (MemberPointer<T> && !MemberFunctionPointer<T>);
}
