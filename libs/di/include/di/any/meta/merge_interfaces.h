#pragma once

#include <di/any/concepts/interface.h>
#include <di/meta/algorithm.h>

namespace di::meta {
template<concepts::Interface... Interfaces>
using MergeInterfaces = meta::Unique<meta::Concat<meta::Transform<Interfaces, meta::Quote<meta::Type>>...>>;
}
