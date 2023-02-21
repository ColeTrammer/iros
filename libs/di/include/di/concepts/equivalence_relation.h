#pragma once

#include <di/concepts/relation.h>

namespace di::concepts {
template<typename R, typename T, typename U>
concept EquivalenceRelation = Relation<R, T, U>;
}
