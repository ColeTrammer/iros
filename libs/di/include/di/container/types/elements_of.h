#pragma once

#include "di/container/concepts/container.h"

namespace di::types {
template<concepts::Container Con>
struct ElementsOf {
    [[no_unique_address]] Con container;
};

template<typename Con>
ElementsOf(Con&&) -> ElementsOf<Con&&>;
}

namespace di {
using types::ElementsOf;
}
