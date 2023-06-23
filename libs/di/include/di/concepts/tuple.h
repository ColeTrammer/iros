#pragma once

#include <di/meta/core.h>
#include <di/meta/remove_cvref.h>
#include <di/vocab/tuple/tuple_forward_declaration.h>

namespace di::concepts {
template<typename T>
concept Tuple = InstanceOf<meta::RemoveCVRef<T>, vocab::Tuple>;
}
