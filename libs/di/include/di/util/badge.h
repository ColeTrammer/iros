#pragma once

namespace di::util {
template<typename T>
class Badge {
private:
    friend T;

    Badge() = default;

public:
    Badge(Badge const&) = delete;
    Badge(Badge&&) = delete;

    auto operator=(Badge const&) -> Badge& = delete;
    auto operator=(Badge&&) -> Badge& = delete;
};
}

namespace di {
using util::Badge;
}
