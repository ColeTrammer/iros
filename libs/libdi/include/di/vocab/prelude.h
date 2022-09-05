#pragma once

#include <di/vocab/array.h>
#include <di/vocab/optional.h>
#include <di/vocab/result.h>
#include <di/vocab/tuple.h>

namespace di {
using vocab::Array;
using vocab::to_array;

using vocab::make_optional;
using vocab::nullopt;
using vocab::Optional;

using vocab::apply;
using vocab::forward_as_tuple;
using vocab::make_decayed_tuple;
using vocab::make_tuple;
using vocab::tie;
using vocab::Tuple;
}
