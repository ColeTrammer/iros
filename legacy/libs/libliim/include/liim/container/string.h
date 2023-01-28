#pragma once

#include <liim/container/strings/ascii_encoding.h>
#include <liim/container/strings/sso_string.h>
#include <liim/container/strings/utf8_encoding.h>

namespace LIIM::Container {
using AsciiString = Strings::SsoStringImpl<Strings::AsciiEncoding>;
using String = Strings::SsoStringImpl<Strings::Utf8Encoding>;
}
