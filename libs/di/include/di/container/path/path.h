#pragma once

#include <di/container/path/path_impl.h>
#include <di/container/string/string.h>
#include <di/container/string/transparent_encoding.h>
#include <di/container/string/utf8_encoding.h>

namespace di::container {
using Path = PathImpl<TransparentString>;
using Utf8Path = PathImpl<String>;
}