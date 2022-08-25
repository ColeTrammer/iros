#pragma once

#include <di/util/concepts/arithmetic.h>
#include <di/util/concepts/enum.h>
#include <di/util/concepts/member_pointer.h>
#include <di/util/concepts/null_pointer.h>
#include <di/util/concepts/pointer.h>

namespace di::util::concepts {
template<typename T>
concept Scalar = (Arithmetic<T> || Enum<T> || Pointer<T> || MemberPointer<T> || NullPointer<T>);
}
