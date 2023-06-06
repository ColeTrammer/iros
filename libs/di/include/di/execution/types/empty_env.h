#pragma once

namespace di::types {
struct EmptyEnv {};

constexpr inline auto empty_env = EmptyEnv {};
}

namespace di {
using types::empty_env;
using types::EmptyEnv;
}
