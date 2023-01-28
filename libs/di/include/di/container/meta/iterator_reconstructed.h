#pragma once

#include <di/container/interface/reconstruct.h>
#include <di/util/declval.h>

namespace di::meta {
template<typename It, typename Sent = It>
using IteratorReconstructed = decltype(container::reconstruct(util::declval<It>(), util::declval<Sent>()));
}