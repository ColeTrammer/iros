#pragma once

#include <di/util/concepts/copy_assignable.h>
#include <di/util/concepts/copy_constructible.h>

namespace di::util::concepts {
template<typename T>
concept Copyable = CopyConstructible<T> && CopyAssignable<T>;
}
