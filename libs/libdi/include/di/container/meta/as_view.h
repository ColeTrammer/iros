#pragma once

#include <di/container/concepts/viewable_container.h>
#include <di/container/view/all.h>
#include <di/util/declval.h>

namespace di::meta {
template<concepts::ViewableContainer Con>
using AsView = decltype(container::view::all(util::declval<Con>()));
}
