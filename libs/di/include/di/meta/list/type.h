#pragma once

#include <di/meta/list/concepts/trait.h>

namespace di::meta {
template<concepts::Trait T>
using Type = T::Type;
}
