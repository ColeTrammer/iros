#pragma once

namespace LIIM {
class StringView;
class String;
}

namespace LIIM::Format {
template<typename T>
struct Formatter;

class FormatArg;

template<typename... Types>
class FormatArgsStorage;

class FormatArgs;
class FormatContext;
class FormatParseContext;

template<typename... Args>
String format(StringView, Args...);
String vformat(StringView, FormatArgs);
}
