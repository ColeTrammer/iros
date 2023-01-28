#pragma once

#include <di/execution/concepts/completion_signature.h>

namespace di::types {
template<concepts::CompletionSignature... Signatures>
struct CompletionSignatures {};
}