#pragma once

#include "di/container/concepts/sentinel_for.h"

namespace di::concepts {
template<typename Sent, typename It>
concept UninitSentinelFor = SentinelFor<Sent, It>;
}
