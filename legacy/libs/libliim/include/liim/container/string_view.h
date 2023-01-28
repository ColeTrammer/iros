#pragma once

#include <liim/container/strings/ascii_encoding.h>
#include <liim/container/strings/string_view.h>
#include <liim/container/strings/utf8_encoding.h>

namespace LIIM::Container {
using AsciiStringView = Strings::StringViewImpl<Strings::AsciiEncoding>;
using StringView = Strings::StringViewImpl<Strings::Utf8Encoding>;
}
