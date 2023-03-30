#pragma once

#include <di/container/string/encoding.h>
#include <di/container/string/erased_string.h>
#include <di/container/string/fixed_string.h>
#include <di/container/string/string.h>
#include <di/container/string/string_view.h>
#include <di/container/string/transparent_encoding.h>
#include <di/container/string/utf8_encoding.h>
#include <di/container/string/zstring.h>

namespace di {
namespace encoding = container::string::encoding;

using container::ErasedString;
using container::FixedString;
using container::String;
using container::StringView;
using container::TransparentString;
using container::TransparentStringView;
using container::ZCString;
using container::ZCUString;
using container::ZCWString;
using container::ZString;
using container::ZUString;
using container::ZWString;
}
