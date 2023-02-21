#pragma once

#include <di/meta/decay.h>
#include <di/vocab/tuple/tuple_forward_declaration.h>

namespace di::meta {
template<typename... Types>
using DecayedTuple = vocab::Tuple<meta::Decay<Types>...>;
}
