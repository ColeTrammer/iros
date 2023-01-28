#pragma once

#include <di/container/concepts/borrowed_container.h>
#include <di/container/concepts/container.h>
#include <di/container/iterator/dangling.h>
#include <di/container/meta/container_iterator.h>
#include <di/container/view/view.h>
#include <di/meta/conditional.h>

namespace di::meta {
template<concepts::Container Con>
using BorrowedView =
    Conditional<concepts::BorrowedContainer<Con>, container::View<ContainerIterator<Con>>, container::Dangling>;
}
