#pragma once

#include <di/concepts/instance_of.h>
#include <di/concepts/same_as.h>
#include <di/execution/types/completion_signuatures.h>

namespace di::concepts {
template<typename Signatures>
concept ValidCompletionSignatures = InstanceOf<Signatures, types::CompletionSignatures>;
}
