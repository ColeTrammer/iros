#pragma once

namespace di::util {
struct Immovable {
    Immovable() = default;

    Immovable(Immovable&&) = delete;
};
}

namespace di {
using util::Immovable;
}
