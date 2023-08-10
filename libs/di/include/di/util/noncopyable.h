#pragma once

namespace di::util {
struct NonCopyable {
    NonCopyable() = default;
    NonCopyable(NonCopyable const&) = delete;
    NonCopyable(NonCopyable&&) = default;
};
}

namespace di {
using util::NonCopyable;
}
