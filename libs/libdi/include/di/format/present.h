#pragma once

#include <di/container/string/transparent_encoding.h>
#include <di/format/present_encoded.h>

namespace di::format {
constexpr inline auto present = present_encoded<container::string::TransparentEncoding>;
}
