#pragma once

#include <di/util/concepts/lvalue_reference.h>
#include <di/util/meta/conditional.h>
#include <di/util/meta/remove_reference.h>
#include <di/util/meta/wrap_reference.h>
#include <di/vocab/optional/basic_optional_storage.h>
#include <di/vocab/optional/optional_storage.h>

namespace di::vocab::optional {
template<typename T>
using StorageFor =
    util::meta::Conditional<OptionalStorage<util::meta::WrapReference<T>, T>, util::meta::WrapReference<T>, BasicOptionalStorage<T>>;
}
