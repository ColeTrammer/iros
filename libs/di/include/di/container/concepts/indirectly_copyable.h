#pragma once

#include <di/container/concepts/indirectly_readable.h>
#include <di/container/concepts/indirectly_writable.h>
#include <di/container/meta/iterator_reference.h>

namespace di::concepts {
template<typename In, typename Out>
concept IndirectlyCopyable = IndirectlyReadable<In> && IndirectlyWritable<Out, meta::IteratorReference<In>>;
}
