#pragma once

#include <stdlib.h>

class StringView {
public:
    StringView(const char* start, const char* end) : m_start(start), m_end(end) {}
    StringView(const StringView& other) : m_start(other.start()), m_end(other.end()) {}

    int size() const { return m_end - m_start + 1; }

    const char* start() const { return m_start; }
    const char* end() const { return m_end; }

private:
    const char* m_start;
    const char* m_end;
};