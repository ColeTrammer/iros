#pragma once

#include <di/concepts/language_void.h>
#include <di/concepts/same_as.h>

namespace di::concepts {
template<typename T>
concept Lock = requires(T& lock) {
                   { lock.lock() } -> LanguageVoid;
                   { lock.try_lock() } -> SameAs<bool>;
                   { lock.unlock() } -> LanguageVoid;
               };
}