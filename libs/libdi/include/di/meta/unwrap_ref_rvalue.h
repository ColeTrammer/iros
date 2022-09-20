#pragma once

#include <di/meta/remove_rvalue_reference.h>
#include <di/meta/unwrap_reference.h>

namespace di::meta {
template<typename T>
using UnwrapRefRValue = meta::RemoveRValueReference<meta::UnwrapReference<T>>;
}