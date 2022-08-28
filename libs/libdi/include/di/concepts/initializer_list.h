#pragma once

#include <di/concepts/same_as.h>
#include <di/meta/remove_cvref.h>
#include <di/util/initializer_list.h>

namespace di::concepts {
template<typename T>
using InitializerList = SameAs<util::InitializerList, meta::RemoveCVRef<T>>;
}
