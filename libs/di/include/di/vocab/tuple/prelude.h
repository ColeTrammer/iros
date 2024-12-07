#pragma once

#include "di/vocab/tuple/apply.h"
#include "di/vocab/tuple/enable_generate_structed_bindings.h"
#include "di/vocab/tuple/forward_as_tuple.h"
#include "di/vocab/tuple/make_decayed_tuple.h"
#include "di/vocab/tuple/make_from_tuple.h"
#include "di/vocab/tuple/make_tuple.h"
#include "di/vocab/tuple/tie.h"
#include "di/vocab/tuple/tuple.h"
#include "di/vocab/tuple/tuple_cat.h"
#include "di/vocab/tuple/tuple_for_each.h"
#include "di/vocab/tuple/tuple_sequence.h"
#include "di/vocab/tuple/tuple_transform.h"

namespace di {
using vocab::apply;
using vocab::forward_as_tuple;
using vocab::make_decayed_tuple;
using vocab::make_from_tuple;
using vocab::make_tuple;
using vocab::tie;
using vocab::Tuple;
using vocab::tuple_cat;
using vocab::tuple_for_each;
using vocab::tuple_sequence;
using vocab::tuple_transform;
}
