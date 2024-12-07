#pragma once

#include "di/container/interface/reconstruct.h"
#include "di/container/meta/container_iterator.h"
#include "di/container/meta/container_sentinel.h"
#include "di/types/prelude.h"
#include "di/util/declval.h"

namespace di::meta {
template<typename T, typename It = ContainerIterator<T>, typename Sent = ContainerSentinel<T>>
using Reconstructed = decltype(container::reconstruct(in_place_type<T>, util::declval<It>(), util::declval<Sent>()));
}
