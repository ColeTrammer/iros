#pragma once

#include <liim/container/strings/ascii_encoding.h>
#include <liim/container/strings/heap_string.h>
#include <liim/container/strings/utf8_encoding.h>

namespace LIIM::Container {
using AsciiHeapString = Strings::HeapStringImpl<Strings::AsciiEncoding>;
using HeapString = Strings::HeapStringImpl<Strings::Utf8Encoding>;
}
