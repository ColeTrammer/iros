#pragma once

#include <di/util/concepts/language_function.h>
#include <di/util/concepts/member_pointer.h>
#include <di/util/meta/member_pointer_value.h>

namespace di::util::concepts {
template<typename T>
concept MemberFunctionPointer = MemberPointer<T> && LanguageFunction<meta::MemberPointerValue<T>>;
}
