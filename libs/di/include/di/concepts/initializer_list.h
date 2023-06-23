#pragma once

#include <di/meta/core.h>
#include <di/meta/remove_cvref.h>
#include <di/util/initializer_list.h>

namespace di::concepts {
template<typename T>
concept InitializerList = InstanceOf<meta::RemoveCVRef<T>, std::initializer_list>;
}
