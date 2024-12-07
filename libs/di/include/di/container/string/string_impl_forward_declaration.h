#pragma once

#include "di/container/vector/mutable_vector.h"
#include "di/container/vector/vector.h"

namespace di::container::string {
template<concepts::Encoding Enc, concepts::detail::MutableVector Vec = Vector<meta::EncodingCodeUnit<Enc>>>
requires(concepts::SameAs<meta::detail::VectorValue<Vec>, meta::EncodingCodeUnit<Enc>>)
class StringImpl;
}
