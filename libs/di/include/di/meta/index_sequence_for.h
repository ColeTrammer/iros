#pragma once

#include <di/meta/make_index_sequence.h>

namespace di::meta {
template<typename... Types>
using IndexSequenceFor = MakeIndexSequence<sizeof...(Types)>;
}
