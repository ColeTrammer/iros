#pragma once

namespace di::sync {
// These values are built-in to GCC and clang.
// See https://clang.llvm.org/docs/LanguageExtensions.html#c11-atomic-builtins.
enum class MemoryOrder : int {
    Relaxed = __ATOMIC_RELAXED,
    Consume = __ATOMIC_CONSUME,
    Acquire = __ATOMIC_ACQUIRE,
    Release = __ATOMIC_RELEASE,
    AcquireRelease = __ATOMIC_ACQ_REL,
    SequentialConsistency = __ATOMIC_SEQ_CST
};
}
