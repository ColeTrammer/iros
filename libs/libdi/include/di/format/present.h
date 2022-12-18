#pragma once

#include <di/container/string/utf8_encoding.h>
#include <di/format/present_encoded.h>

namespace di::format {
constexpr inline auto present = present_encoded<container::string::Utf8Encoding>;
}
