#pragma once

#include <di/container/concepts/indirect_binary_predicate.h>
#include <di/container/meta/projected.h>
#include <di/function/identity.h>

namespace di::concepts {
template<typename It, typename Jt, typename Comp, typename Proj1 = function::Identity,
         typename Proj2 = function::Identity>
concept IndirectlyComparable = IndirectBinaryPredicate<Comp, meta::Projected<It, Proj1>, meta::Projected<Jt, Proj2>>;
}
