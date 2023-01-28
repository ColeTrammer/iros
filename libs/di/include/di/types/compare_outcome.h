#pragma once

namespace di::types::detail {
// NOTE: this is the same layout libstdc++ uses, so don't change this. The compiler might rely on it.
enum class CompareOutcome : char { Less = -1, Equal = 0, Greater = 1, Unordered = 2 };
}
