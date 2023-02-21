#pragma once

namespace di::types {
template<concepts::Container Con>
struct ElementsOf {
    [[no_unique_address]] Con container;
};

template<typename Con>
ElementsOf(Con&&) -> ElementsOf<Con&&>;
}
