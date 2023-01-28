#pragma once

#include <di/container/concepts/forward_iterator.h>
#include <di/container/concepts/indirectly_movable_storable.h>
#include <di/container/concepts/indirectly_swappable.h>

namespace di::concepts {
template<typename Iter>
concept Permutable = ForwardIterator<Iter> && IndirectlyMovableStorable<Iter, Iter> && IndirectlySwappable<Iter, Iter>;
}