#pragma once

#include <di/types/in_place.h>
#include <di/types/in_place_index.h>
#include <di/types/in_place_template.h>
#include <di/types/in_place_type.h>
#include <di/types/integers.h>
#include <di/types/nullptr_t.h>
#include <di/types/partial_ordering.h>
#include <di/types/piecewise_construct.h>
#include <di/types/ptrdiff_t.h>
#include <di/types/size_t.h>
#include <di/types/ssize_t.h>
#include <di/types/strong_ordering.h>
#include <di/types/void.h>
#include <di/types/weak_ordering.h>

namespace di {
using types::in_place;
using types::in_place_index;
using types::in_place_template;
using types::in_place_type;
using types::piecewise_construct;

using types::InPlace;
using types::InPlaceIndex;
using types::InPlaceTemplate;
using types::InPlaceType;
using types::nullptr_t;
using types::partial_ordering;
using types::PiecewiseConstruct;
using types::ptrdiff_t;
using types::size_t;
using types::ssize_t;
using types::strong_ordering;
using types::Void;
using types::weak_ordering;
}
