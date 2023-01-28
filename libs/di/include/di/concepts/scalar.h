#pragma once

#include <di/concepts/arithmetic.h>
#include <di/concepts/enum.h>
#include <di/concepts/member_pointer.h>
#include <di/concepts/null_pointer.h>
#include <di/concepts/pointer.h>

namespace di::concepts {
template<typename T>
concept Scalar = (Arithmetic<T> || Enum<T> || Pointer<T> || MemberPointer<T> || NullPointer<T>);
}
