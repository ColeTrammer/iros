#pragma once

#include <di/container/string/string_view_impl.h>
#include <di/container/string/transparent_encoding.h>

namespace di::container {
using StringView = string::StringViewImpl<string::TransparentEncoding>;
}
