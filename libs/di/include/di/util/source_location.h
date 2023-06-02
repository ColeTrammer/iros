#pragma once

#include <di/platform/compiler.h>

#ifndef DI_CLANG
#define __builtin_COLUMN() 0
#endif

namespace di::util {
class SourceLocation {
public:
    consteval static SourceLocation current(char const* file_name = __builtin_FILE(),
                                            char const* function_name = __builtin_FUNCTION(),
                                            unsigned int line = __builtin_LINE(),
                                            unsigned int column = __builtin_COLUMN()) {
        auto result = SourceLocation();
        result.m_file_name = file_name;
        result.m_function_name = function_name;
        result.m_line = line;
        result.m_column = column;
        return result;
    }

    constexpr SourceLocation() = default;

    constexpr unsigned int line() const { return m_line; }
    constexpr unsigned int column() const { return m_column; }
    constexpr char const* file_name() const { return m_file_name; }
    constexpr char const* function_name() const { return m_function_name; }

private:
    char const* m_file_name { "" };
    char const* m_function_name { "" };
    unsigned int m_line { 0u };
    unsigned int m_column { 0u };
};
}

#ifndef DI_CLANG
#undef __builtin_COLUMN
#endif
