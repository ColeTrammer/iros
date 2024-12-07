#pragma once

#include "di/container/interface/reconstruct.h"
#include "di/container/meta/container_iterator.h"
#include "di/container/meta/container_sentinel.h"
#include "di/types/prelude.h"
#include "di/util/forward.h"

namespace di::concepts {
template<typename Con, typename It = meta::ContainerIterator<Con>, typename Sent = meta::ContainerSentinel<Con>>
concept ReconstructibleContainer = requires(It iterator, Sent sentinel) {
    container::reconstruct(in_place_type<Con>, util::forward<It>(iterator), util::forward<Sent>(sentinel));
};
}
