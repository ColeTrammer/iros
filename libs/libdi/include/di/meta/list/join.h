#pragma once

#include <di/meta/list/apply.h>
#include <di/meta/list/concat.h>

namespace di::meta {
template<concepts::TypeList List>
using Join = Apply<Quote<Concat>, List>;
}