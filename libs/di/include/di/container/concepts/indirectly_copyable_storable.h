#pragma once

#include <di/concepts/assignable_from.h>
#include <di/concepts/constructible_from.h>
#include <di/concepts/copyable.h>
#include <di/container/concepts/indirectly_copyable.h>
#include <di/container/concepts/indirectly_writable.h>
#include <di/container/meta/iterator_value.h>

namespace di::concepts {
template<typename In, typename Out>
concept IndirectlyCopyableStorable =
    IndirectlyCopyable<In, Out> && IndirectlyWritable<Out, meta::IteratorValue<In>&> &&
    IndirectlyWritable<Out, meta::IteratorValue<In> const&> && IndirectlyWritable<Out, meta::IteratorValue<In>&&> &&
    IndirectlyWritable<Out, meta::IteratorValue<In> const&&> && Copyable<meta::IteratorValue<In>> &&
    ConstructibleFrom<meta::IteratorValue<In>, meta::IteratorReference<In>> &&
    AssignableFrom<meta::IteratorValue<In>&, meta::IteratorReference<In>>;
}
