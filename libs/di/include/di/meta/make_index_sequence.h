#pragma once

#include <di/meta/make_integer_sequence.h>
#include <di/types/size_t.h>

namespace di::meta {
template<types::size_t count>
using MakeIndexSequence = MakeIntegerSequence<types::size_t, count>;
}
