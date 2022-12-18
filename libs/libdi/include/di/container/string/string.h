#pragma once

#include <di/container/string/string_impl.h>
#include <di/container/string/transparent_encoding.h>
#include <di/container/string/utf8_encoding.h>

namespace di::container {
using String = string::StringImpl<string::Utf8Encoding>;
using TransparentString = string::StringImpl<string::TransparentEncoding>;
}
