#pragma once

#include <di/container/string/mutable_string_interface.h>
#include <di/container/string/string_view_impl.h>
#include <di/container/vector/mutable_vector.h>
#include <di/container/vector/vector.h>

namespace di::container::string {
template<concepts::Encoding Enc, concepts::detail::MutableVector Vec = Vector<meta::EncodingCodeUnit<Enc>>>
requires(concepts::SameAs<meta::detail::VectorValue<Vec>, meta::EncodingCodeUnit<Enc>>)
class StringImpl;
}