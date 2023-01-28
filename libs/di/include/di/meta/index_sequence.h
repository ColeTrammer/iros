#pragma once

#include <di/meta/integer_sequence.h>
#include <di/types/size_t.h>

namespace di::meta {
template<types::size_t... values>
using IndexSequence = IntegerSequence<types::size_t, values...>;
}
