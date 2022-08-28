#pragma once

#include <di/concepts/language_function.h>
#include <di/concepts/member_pointer.h>
#include <di/meta/member_pointer_value.h>

namespace di::concepts {
template<typename T>
concept MemberFunctionPointer = MemberPointer<T> && LanguageFunction<meta::MemberPointerValue<T>>;
}
