#pragma once

#include <di/concepts/lvalue_reference.h>
#include <di/meta/core.h>
#include <di/meta/remove_reference.h>
#include <di/meta/wrap_reference.h>
#include <di/vocab/optional/basic_optional_storage.h>
#include <di/vocab/optional/optional_storage.h>

namespace di::vocab {
template<typename T>
using StorageFor =
    meta::Conditional<OptionalStorage<meta::WrapReference<T>, T>, meta::WrapReference<T>, BasicOptionalStorage<T>>;
}
