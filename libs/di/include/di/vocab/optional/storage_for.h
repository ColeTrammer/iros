#pragma once

#include <di/meta/core.h>
#include <di/meta/language.h>
#include <di/meta/vocab.h>
#include <di/vocab/optional/basic_optional_storage.h>
#include <di/vocab/optional/optional_storage.h>

namespace di::vocab {
template<typename T>
using StorageFor =
    meta::Conditional<OptionalStorage<meta::WrapReference<T>, T>, meta::WrapReference<T>, BasicOptionalStorage<T>>;
}
