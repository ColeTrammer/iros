#pragma once

#include <di/concepts/copy_assignable.h>
#include <di/concepts/copy_constructible.h>

namespace di::concepts {
template<typename T>
concept Copyable = CopyConstructible<T> && CopyAssignable<T>;
}
