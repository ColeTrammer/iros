#pragma once

#include <di/vocab/tuple/enable_generate_structed_bindings.h>
#include <di/vocab/tuple/forward_as_tuple.h>
#include <di/vocab/tuple/make_tuple.h>
#include <di/vocab/tuple/make_decayed_tuple.h>
#include <di/vocab/tuple/tie.h>
#include <di/vocab/tuple/tuple.h>
#include <di/vocab/tuple/apply.h>

namespace di::vocab {
using tuple::apply;
using tuple::forward_as_tuple;
using tuple::make_decayed_tuple;
using tuple::make_tuple;
using tuple::tie;
using tuple::Tuple;
}
