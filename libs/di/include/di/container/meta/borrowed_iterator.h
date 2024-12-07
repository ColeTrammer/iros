#pragma once

#include "di/container/concepts/borrowed_container.h"
#include "di/container/concepts/container.h"
#include "di/container/iterator/dangling.h"
#include "di/container/meta/container_iterator.h"
#include "di/meta/core.h"

namespace di::meta {
template<concepts::Container Con>
using BorrowedIterator = Conditional<concepts::BorrowedContainer<Con>, ContainerIterator<Con>, container::Dangling>;
}
