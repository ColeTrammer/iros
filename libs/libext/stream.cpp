#include <ext/stream.h>

namespace Ext {
Stream::Stream() {}

Stream::~Stream() {}

Generator<StreamResult> Stream::read_bits(uint32_t& bits, uint8_t bit_count) {
    bits = 0;
    for (uint8_t i = 0; i < bit_count;) {
        auto bit = m_reader.next_bit();
        if (!bit) {
            co_yield StreamResult::NeedsMoreInput;
            continue;
        }

        if (bit) {
            bits |= (1U << i);
        }
        i++;
    }
}

Generator<StreamResult> Stream::read_bytes(Span<uint8_t> bytes) {
    for (size_t i = 0; i < bytes.size();) {
        auto maybe_byte = m_reader.next_byte();
        if (!maybe_byte) {
            co_yield StreamResult::NeedsMoreInput;
            continue;
        }

        bytes[i++] = maybe_byte.value();
    }
}

Generator<StreamResult> Stream::read_null_terminated_string(String& string) {
    for (;;) {
        auto maybe_byte = m_reader.next_byte();
        if (!maybe_byte) {
            co_yield StreamResult::NeedsMoreInput;
            continue;
        }

        auto byte = maybe_byte.value();
        if (byte == '\0') {
            break;
        }

        string += String(byte);
    }
}

Generator<StreamResult> Stream::write_bits(uint32_t bits, uint8_t bit_count) {
    for (uint8_t i = 0; i < bit_count;) {
        if (!m_writer.write_bit(!!(bits & (1U << i)))) {
            co_yield StreamResult::NeedsMoreOutputSpace;
            continue;
        }
        i++;
    }
}

Generator<StreamResult> Stream::write_bytes(Span<const uint8_t> bytes) {
    for (size_t i = 0; i < bytes.size();) {
        if (!m_writer.write_byte(bytes[i])) {
            co_yield StreamResult::NeedsMoreOutputSpace;
            continue;
        }
        i++;
    }
}

Generator<StreamResult> Stream::write_null_terminated_string(const String& string) {
    return write_bytes({ reinterpret_cast<const uint8_t*>(string.string()), string.size() + 1 });
}
}
