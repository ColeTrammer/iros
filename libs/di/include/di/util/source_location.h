#pragma once

#include <di/platform/compiler.h>

#ifndef DI_CLANG
#define __builtin_COLUMN() 0
#endif

namespace di::util {
class SourceLocation {
public:
    consteval static auto current(char const* file_name = __builtin_FILE(),
                                  char const* function_name = __builtin_FUNCTION(),
                                  unsigned int line = __builtin_LINE(), unsigned int column = __builtin_COLUMN())
        -> SourceLocation {
        auto result = SourceLocation();
        result.m_file_name = file_name;
        result.m_function_name = function_name;
        result.m_line = line;
        result.m_column = column;
        return result;
    }

    constexpr SourceLocation() = default;

    constexpr auto line() const -> unsigned int { return m_line; }
    constexpr auto column() const -> unsigned int { return m_column; }
    constexpr auto file_name() const -> char const* { return m_file_name; }
    constexpr auto function_name() const -> char const* { return m_function_name; }

private:
    char const* m_file_name { "" };
    char const* m_function_name { "" };
    unsigned int m_line { 0U };
    unsigned int m_column { 0U };
};
}

namespace di {
using util::SourceLocation;
}

#ifndef DI_CLANG
#undef __builtin_COLUMN
#endif
