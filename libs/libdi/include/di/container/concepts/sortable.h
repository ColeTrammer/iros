#pragma once

#include <di/container/concepts/indirect_strict_weak_order.h>
#include <di/container/concepts/permutable.h>
#include <di/container/meta/projected.h>
#include <di/function/identity.h>
#include <di/function/less.h>

namespace di::concepts {
template<typename Iter, typename Comp = function::Less, typename Proj = function::Identity>
concept Sortable = Permutable<Iter> && IndirectStrictWeakOrder<Comp, meta::Projected<Iter, Proj>>;
}