#pragma once

#include <liim/string_view.h>
#include <source_location>

#ifndef __clang__
#define __builtin_COLUMN() 0
#endif

namespace LIIM {
class SourceLocation {
public:
    constexpr unsigned int line() const { return m_line; }
    constexpr unsigned int column() const { return m_column; }
    constexpr StringView file_name() const { return m_file_name; }
    constexpr StringView function_name() const { return m_function_name; }

    static constexpr SourceLocation current(unsigned int line = __builtin_LINE(), unsigned int column = __builtin_COLUMN(),
                                            StringView file_name = __builtin_FILE(), StringView function_name = __builtin_FUNCTION()) {
        return SourceLocation(line, column, file_name, function_name);
    }

    constexpr SourceLocation() = default;

private:
    constexpr SourceLocation(unsigned int line, unsigned int column, StringView file_name, StringView function_name)
        : m_line(line), m_column(column), m_file_name(file_name), m_function_name(function_name) {}

    unsigned int m_line { 0 };
    unsigned int m_column { 0 };
    StringView m_file_name;
    StringView m_function_name;
};
}

using LIIM::SourceLocation;
