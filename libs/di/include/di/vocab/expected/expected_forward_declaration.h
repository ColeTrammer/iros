#pragma once

namespace di::vocab {
template<typename E>
class Unexpected;

template<typename T, typename E>
class [[nodiscard]] Expected;
}

namespace di {
using vocab::Expected;
using vocab::Unexpected;
}
