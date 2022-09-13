#pragma once

#include <di/container/string/string_impl.h>
#include <di/container/string/transparent_encoding.h>

namespace di::container {
using String = string::StringImpl<string::TransparentEncoding>;
}
