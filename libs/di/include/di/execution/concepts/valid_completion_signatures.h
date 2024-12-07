#pragma once

#include "di/execution/types/completion_signuatures.h"
#include "di/meta/core.h"

namespace di::concepts {
template<typename Signatures>
concept ValidCompletionSignatures = InstanceOf<Signatures, types::CompletionSignatures>;
}
