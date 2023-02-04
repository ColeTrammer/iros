#pragma once

#include <di/concepts/language_function.h>

namespace di::types {
template<typename T, concepts::LanguageFunction S>
struct Method {
    using Type = Method;
    using Tag = T;
    using Signature = S;
};
}