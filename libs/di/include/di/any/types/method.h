#pragma once

#include <di/meta/language.h>

namespace di::types {
template<typename T, concepts::LanguageFunction S>
struct Method {
    using Type = Method;
    using Tag = T;
    using Signature = S;
};
}

namespace di {
using types::Method;
}
