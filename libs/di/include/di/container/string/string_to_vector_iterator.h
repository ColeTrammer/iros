#pragma once

#include <di/container/string/encoding.h>
#include <di/container/string/mutable_string.h>

namespace di::container::string {
template<concepts::detail::MutableString Str, typename Enc = meta::Encoding<Str>,
         typename It = meta::EncodingIterator<Enc>>
constexpr auto string_to_vector_iterator(Str& string, It it) {
    return encoding::iterator_data(string.encoding(), string.span(), it);
}
}
