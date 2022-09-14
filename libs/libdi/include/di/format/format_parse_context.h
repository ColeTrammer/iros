#pragma once

#include <di/container/string/encoding.h>

namespace di::concepts {
template<typename T>
concept FormatParseContext = requires { typename meta::Encoding<T>; };
}

namespace di::format {
struct ParseContextPlaceholder {
    using Encoding = void;
};
}
