#pragma once

namespace di::util {
struct Immovable {
    Immovable() = default;

private:
    Immovable(Immovable&&) = delete;
};
}
