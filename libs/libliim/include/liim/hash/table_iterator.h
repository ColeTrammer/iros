#pragma once

#include <liim/hash/group_info.h>

namespace LIIM::Hash::Detail {
template<typename Table>
class TableIterator {
private:
    Table& m_table;
    size_t m_group_index { 0 };
    uint8_t m_index_into_group { 0 };

    constexpr TableIterator(Table& table, size_t group_index, uint8_t index_into_group)
        : m_table(table), m_group_index(group_index), m_index_into_group(index_into_group) {}

    constexpr TableIterator(Table& table) : m_table(table) {
        if (table.group_count() != 0 && !table.m_groups[0].entry(0).present()) {
            ++*this;
        }
    }

public:
    constexpr operator TableIterator<const Table>() const requires(!IsConst<Table>::value) {
        return TableIterator<const Table>(m_table, m_group_index, m_index_into_group);
    }

    constexpr decltype(auto) operator*() { return m_table.value(m_table.value_index(m_group_index, m_index_into_group)); }

    using ValueType = decltype(*declval<TableIterator>());

    constexpr TableIterator& operator++() {
        for (;;) {
            if (++m_index_into_group == GroupInfo::entry_count) {
                m_index_into_group = 0;
                if (++m_group_index == m_table.group_count()) {
                    break;
                }
            }
            if (m_table.m_groups[m_group_index].entry(m_index_into_group).present()) {
                break;
            }
        }
        return *this;
    }

    constexpr TableIterator operator++(int) {
        auto result = TableIterator(*this);
        ++*this;
        return result;
    }

    constexpr bool operator==(const TableIterator& other) const {
        return this->m_group_index == other.m_group_index && this->m_index_into_group == other.m_index_into_group;
    }

    friend Table;
    friend TableIterator<typename RemoveConst<Table>::type>;
};
}
