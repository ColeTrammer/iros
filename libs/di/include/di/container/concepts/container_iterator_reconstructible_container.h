#pragma once

#include <di/container/interface/reconstruct.h>
#include <di/container/meta/container_iterator.h>
#include <di/container/meta/container_sentinel.h>
#include <di/meta/remove_cvref.h>
#include <di/types/prelude.h>
#include <di/util/forward.h>

namespace di::concepts {
template<typename Con, typename Tag = meta::RemoveCVRef<Con>, typename It = meta::ContainerIterator<Con>,
         typename Sent = meta::ContainerSentinel<Con>>
concept ContainerIteratorReconstructibleContainer =
    requires(Con container, It iterator, Sent sentinel) {
        container::reconstruct(in_place_type<Tag>, util::forward<Con>(container), util::forward<It>(iterator),
                               util::forward<Sent>(sentinel));
    };
}
