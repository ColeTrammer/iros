#pragma once

namespace di::any {
enum class StorageCategory {
    Reference,
    Trivial,
    TriviallyRelocatable,
    Immovable,
    MoveOnly,
    Copyable,
    InfalliblyCloneable,
    Cloneable,
};
}
