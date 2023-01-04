#pragma once

#include <di/meta/list/as_template.h>
#include <di/vocab/tuple/tuple_forward_declaration.h>

namespace di::meta {
template<concepts::TypeList T>
using AsTuple = AsTemplate<vocab::Tuple, T>;
}