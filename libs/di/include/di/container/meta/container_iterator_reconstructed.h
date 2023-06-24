#pragma once

#include <di/container/interface/reconstruct.h>
#include <di/container/meta/container_iterator.h>
#include <di/container/meta/container_sentinel.h>
#include <di/meta/core.h>
#include <di/types/prelude.h>
#include <di/util/declval.h>

namespace di::meta {
template<typename T, typename Tag = meta::RemoveCVRef<T>, typename It = ContainerIterator<T>,
         typename Sent = ContainerSentinel<T>>
using ContainerIteratorReconstructed =
    decltype(container::reconstruct(in_place_type<Tag>, util::declval<T>, util::declval<It>(), util::declval<Sent>()));
}
