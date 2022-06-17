#pragma once

#include <liim/container/hash/set.h>

namespace LIIM::Container {
template<Hashable T>
using HashSet = Hash::Set<T>;
using Hash::collect_hash_set;
using Hash::make_hash_set;
}

using LIIM::Container::collect_hash_set;
using LIIM::Container::make_hash_set;