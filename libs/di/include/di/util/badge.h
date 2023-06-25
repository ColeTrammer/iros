#pragma once

namespace di::util {
template<typename T>
class Badge {
private:
    friend T;

    Badge() = default;

    Badge(Badge const&) = delete;
    Badge(Badge&&) = delete;

    Badge& operator=(Badge const&) = delete;
    Badge& operator=(Badge&&) = delete;
};
}

namespace di {
using util::Badge;
}
