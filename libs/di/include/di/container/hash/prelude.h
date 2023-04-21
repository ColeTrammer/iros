#pragma once

#include <di/container/hash/default_hasher.h>
#include <di/container/hash/hash.h>
#include <di/container/hash/hash_write.h>
#include <di/container/hash/hasher.h>

namespace di {
using concepts::Hashable;
using concepts::Hasher;

using container::DefaultHasher;

using container::hash;
using container::hash_write;
}
