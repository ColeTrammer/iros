#pragma once

#include <di/concepts/assignable_from.h>
#include <di/concepts/constructible_from.h>
#include <di/concepts/movable.h>
#include <di/container/concepts/indirectly_movable.h>
#include <di/container/concepts/indirectly_writable.h>
#include <di/container/meta/iterator_rvalue.h>
#include <di/container/meta/iterator_value.h>

namespace di::concepts {
template<typename In, typename Out>
concept IndirectlyMovableStorable =
    IndirectlyMovable<In, Out> && IndirectlyWritable<Out, meta::IteratorValue<In>> &&
    Movable<meta::IteratorValue<In>> && ConstructibleFrom<meta::IteratorValue<In>, meta::IteratorRValue<In>> &&
    AssignableFrom<meta::IteratorValue<In>&, meta::IteratorRValue<In>>;
}
