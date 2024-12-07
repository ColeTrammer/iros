#pragma once

#include "di/container/concepts/indirectly_unary_invocable.h"

namespace di::concepts {
template<typename F, typename It>
concept IndirectlyRegularUnaryInvocable = IndirectlyUnaryInvocable<F, It>;
}
