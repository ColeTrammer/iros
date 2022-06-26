#pragma once

#include <liim/container/hash.h>
#include <liim/container/hash/map.h>

namespace LIIM::Container {
template<typename K, typename V>
using HashMap = Hash::Map<K, V>;
using Hash::collect_hash_map;
using Hash::make_hash_map;
}

using LIIM::Container::collect_hash_map;
using LIIM::Container::make_hash_map;
