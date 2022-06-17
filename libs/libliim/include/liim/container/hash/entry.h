#pragma once

#include <liim/container/hash/entry_info.h>

namespace LIIM::Container::Hash::Detail {
class Entry {
public:
    constexpr Entry(EntryInfo& info, size_t value_index) : m_info(&info), m_value_index(value_index) {}

    constexpr EntryInfo& info() { return *m_info; }
    constexpr const EntryInfo& info() const { return *m_info; }
    constexpr size_t value_index() const { return m_value_index; }

private:
    EntryInfo* m_info { nullptr };
    size_t m_value_index { 0 };
};
}
