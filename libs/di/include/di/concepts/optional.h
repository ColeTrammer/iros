#pragma once

#include <di/meta/core.h>
#include <di/meta/remove_cvref.h>
#include <di/vocab/optional/optional_forward_declaration.h>

namespace di::concepts {
template<typename T>
concept Optional = InstanceOf<meta::RemoveCVRef<T>, vocab::Optional>;
}
