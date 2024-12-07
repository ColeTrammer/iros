#pragma once

#include "di/meta/core.h"
#include "di/meta/language.h"

namespace di::concepts {
template<typename T>
concept Lock = requires(T& lock) {
    { lock.lock() } -> LanguageVoid;
    { lock.try_lock() } -> SameAs<bool>;
    { lock.unlock() } -> LanguageVoid;
};
}
