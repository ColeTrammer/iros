#pragma once

#include <liim/container/hash/entry_info.h>

namespace LIIM::Container::Hash::Detail {
class GroupInfo {
public:
    constexpr GroupInfo() {}

    static constexpr uint32_t entry_count = 8;

    constexpr uint8_t present_entries() const {
        uint8_t result = 0;
        for (uint32_t i = 0; i < entry_count; i++) {
            result |= m_info[i].present() << i;
        }
        return result;
    }

    constexpr uint8_t tombstone_entries() const {
        uint8_t result = 0;
        for (uint32_t i = 0; i < entry_count; i++) {
            result |= m_info[i].tombstone() << i;
        }
        return result;
    }

    constexpr EntryInfo& entry(uint8_t index) { return m_info[index]; }
    constexpr const EntryInfo& entry(uint8_t index) const { return m_info[index]; }

private:
    EntryInfo m_info[entry_count] {};
};
}
