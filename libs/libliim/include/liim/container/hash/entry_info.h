#pragma once

#include <assert.h>
#include <stdint.h>

namespace LIIM::Container::Hash::Detail {
class EntryInfo {
public:
    constexpr EntryInfo() {}

    constexpr bool present() const { return !!(m_metadata & 0x80); }
    constexpr uint8_t hash_low() const {
        assert(present());
        return m_metadata & 0x7F;
    }

    constexpr bool vacant() const { return m_metadata == 0x00; }
    constexpr bool tombstone() const { return m_metadata == 0x01; }

    constexpr void set_present(uint8_t hash_low) {
        assert(!(hash_low & 0x80));
        m_metadata = 0x80 | hash_low;
    }

    constexpr void set_vacant() { m_metadata = 0x00; }
    constexpr void set_tombstone() { m_metadata = 0x01; }

private:
    uint8_t m_metadata { 0 };
};
}
