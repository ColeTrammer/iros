#pragma once

#include <di/container/concepts/indirectly_writable.h>
#include <di/container/concepts/iterator.h>
#include <di/util/forward.h>

namespace di::concepts {
template<typename It, typename T>
concept OutputIterator =
    Iterator<It> && IndirectlyWritable<It, T> && requires(It it, T&& value) { *it++ = util::forward<T>(value); };
}