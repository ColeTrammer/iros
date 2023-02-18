#pragma once

#include <di/bit/endian/prelude.h>
#include <di/format/prelude.h>
#include <di/parser/prelude.h>
#include <di/random/prelude.h>
#include <di/types/prelude.h>
#include <di/util/bit_cast.h>
#include <di/util/to_underlying.h>
#include <di/vocab/array/prelude.h>

namespace di::util {
namespace detail {
    struct GenerateUUIDFunction;
}

class UUID {
private:
    using ByteArray = di::Array<di::Byte, 16>;

    enum class Variant : u8 {
        LittleEndian = 0b110,
    };

    enum class Version : u8 {
        Time = 0b0001,
        DCESecurity = 0b0010,
        NameMD5 = 0b0011,
        Random = 0b0100,
        NameSHA1 = 0b0101,
    };

public:
    UUID() = default;

    constexpr explicit UUID(ByteArray bytes) { *this = util::bit_cast<UUID>(bytes); }

    constexpr bool null() const { return *this == UUID(); }
    constexpr void clear() { *this = UUID(); }

private:
    friend struct detail::GenerateUUIDFunction;

    constexpr friend bool operator==(UUID a, UUID b) {
        return util::bit_cast<ByteArray>(a) == util::bit_cast<ByteArray>(b);
    }
    constexpr friend strong_ordering operator<=>(UUID a, UUID b) {
        return util::bit_cast<ByteArray>(a) <=> util::bit_cast<ByteArray>(b);
    }

    constexpr Variant variant() const { return Variant(m_clock_seq_hi_and_res >> 5); }
    constexpr Version version() const { return Version(m_time_hi_and_version >> 4); }

    constexpr void set_to_standard_variant() {
        m_clock_seq_hi_and_res &= 0b11000000;
        m_clock_seq_hi_and_res |= 0b10000000;
    }

    constexpr void set_version(Version type) {
        m_time_hi_and_version &= 0b00001111;
        m_time_hi_and_version |= util::to_underlying(type) << 4;
    }

    constexpr bool is_little_endian() const { return variant() == Variant::LittleEndian; }

    template<concepts::Encoding Enc>
    constexpr friend auto tag_invoke(types::Tag<format::formatter_in_place>, InPlaceType<UUID>,
                                     FormatParseContext<Enc>& parse_context, bool debug) {
        return format::formatter<container::TransparentStringView, Enc>(parse_context, debug) %
               [](concepts::CopyConstructible auto formatter) {
                   return [=](concepts::FormatContext auto& context, UUID uuid) {
                       auto buffer = di::Array<char, 36> {};
                       usize index = 0;

                       auto output_digit = [&](u8 digit) {
                           if (digit >= 10) {
                               buffer[index++] = digit - 10 + 'a';
                           } else {
                               buffer[index++] = digit + '0';
                           }
                       };

                       auto output_byte = [&](auto byte) {
                           output_digit(byte >> 4);
                           output_digit(byte & 0xF);
                       };

                       auto output = [&](auto value, auto bytes) {
                           u64 mask = 0xFF << 8 * (bytes - 1);
                           for (; bytes > 0; bytes--) {
                               u8 byte = (value & mask) >> 8 * (bytes - 1);
                               mask >>= 8;

                               output_byte(byte);
                           }
                       };

                       if (uuid.is_little_endian()) {
                           output(little_endian_to_host(uuid.m_time_low), 4);
                       } else {
                           output(big_endian_to_host(uuid.m_time_low), 4);
                       }
                       buffer[index++] = '-';
                       if (uuid.is_little_endian()) {
                           output(little_endian_to_host(uuid.m_time_mid), 2);
                       } else {
                           output(big_endian_to_host(uuid.m_time_mid), 2);
                       }
                       buffer[index++] = '-';
                       if (uuid.is_little_endian()) {
                           output(little_endian_to_host(uuid.m_time_hi_and_version), 2);
                       } else {
                           output(big_endian_to_host(uuid.m_time_hi_and_version), 2);
                       }
                       buffer[index++] = '-';
                       output_byte(uuid.m_clock_seq_hi_and_res);
                       output_byte(uuid.m_clock_seq_low);
                       buffer[index++] = '-';
                       output_byte(util::to_underlying(uuid.m_node[0]));
                       output_byte(util::to_underlying(uuid.m_node[1]));
                       output_byte(util::to_underlying(uuid.m_node[2]));
                       output_byte(util::to_underlying(uuid.m_node[3]));
                       output_byte(util::to_underlying(uuid.m_node[4]));
                       output_byte(util::to_underlying(uuid.m_node[5]));

                       return formatter(context, container::TransparentStringView(buffer.begin(), buffer.end()));
                   };
               };
    }

    constexpr friend auto tag_invoke(types::Tag<parser::create_parser_in_place>, InPlaceType<UUID>) {
        auto valid_hex = ('0'_m - '9'_m || 'a'_m - 'f'_m || 'A'_m - 'F'_m);
        auto minus = '-'_m;

        return (parser::match_exactly(valid_hex, 8) >> ~parser::match_one(minus) >>
                parser::match_exactly(valid_hex, 4) >> ~parser::match_one(minus) >>
                parser::match_exactly(valid_hex, 4) >> ~parser::match_one(minus) >>
                parser::match_exactly(valid_hex, 4) >> ~parser::match_one(minus) >>
                parser::match_exactly(valid_hex, 12))
                   << []<typename Context>(
                          Context& context,
                          concepts::CopyConstructible auto results) -> meta::ParserContextResult<UUID, Context> {
            using Enc = meta::Encoding<Context>;
            auto encoding = context.encoding();

            auto [a, b, c, d, e] = results;
            u32 m_time_low = parser::run_parser_unchecked(
                parser::integer<u32, 16>(),
                container::string::StringViewImpl<Enc> { encoding::unicode_code_point_unwrap(encoding, a.begin()),
                                                         encoding::unicode_code_point_unwrap(encoding, a.end()) });
            u16 m_time_mid = parser::run_parser_unchecked(
                parser::integer<u16, 16>(),
                container::string::StringViewImpl<Enc> { encoding::unicode_code_point_unwrap(encoding, b.begin()),
                                                         encoding::unicode_code_point_unwrap(encoding, b.end()) });
            u16 time_hi = parser::run_parser_unchecked(
                parser::integer<u16, 16>(),
                container::string::StringViewImpl<Enc> { encoding::unicode_code_point_unwrap(encoding, c.begin()),
                                                         encoding::unicode_code_point_unwrap(encoding, c.end()) });
            u16 clock_seq = parser::run_parser_unchecked(
                parser::integer<u16, 16>(),
                container::string::StringViewImpl<Enc> { encoding::unicode_code_point_unwrap(encoding, d.begin()),
                                                         encoding::unicode_code_point_unwrap(encoding, d.end()) });
            u64 m_node = parser::run_parser_unchecked(
                parser::integer<u64, 16>(),
                container::string::StringViewImpl<Enc> { encoding::unicode_code_point_unwrap(encoding, e.begin()),
                                                         encoding::unicode_code_point_unwrap(encoding, e.end()) });

            auto result = UUID {};
            result.m_time_low = m_time_low;
            result.m_time_mid = m_time_mid;
            result.m_time_hi_and_version = time_hi;
            result.m_clock_seq_hi_and_res = clock_seq >> 8;
            result.m_clock_seq_low = clock_seq & 0xFF;
            result.m_node[0] = Byte((m_node & 0x0000FF0000000000) >> 40);
            result.m_node[1] = Byte((m_node & 0x000000FF00000000) >> 32);
            result.m_node[2] = Byte((m_node & 0x00000000FF000000) >> 24);
            result.m_node[3] = Byte((m_node & 0x0000000000FF0000) >> 16);
            result.m_node[4] = Byte((m_node & 0x000000000000FF00) >> 8);
            result.m_node[5] = Byte((m_node & 0x00000000000000FF) >> 0);

            if (result.is_little_endian()) {
                result.m_time_low = host_to_little_endian(result.m_time_low);
                result.m_time_mid = host_to_little_endian(result.m_time_mid);
                result.m_time_hi_and_version = host_to_little_endian(result.m_time_hi_and_version);
            } else {
                result.m_time_low = host_to_big_endian(result.m_time_low);
                result.m_time_mid = host_to_big_endian(result.m_time_mid);
                result.m_time_hi_and_version = host_to_big_endian(result.m_time_hi_and_version);
            }
            return result;
        };
    }

    u32 m_time_low { 0 };
    u16 m_time_mid { 0 };
    u16 m_time_hi_and_version { 0 };
    u8 m_clock_seq_hi_and_res { 0 };
    u8 m_clock_seq_low { 0 };
    di::Array<di::Byte, 6> m_node { { di::Byte(0), di::Byte(0), di::Byte(0), di::Byte(0), di::Byte(0), di::Byte(0) } };
};

namespace detail {
    struct GenerateUUIDFunction {
        template<concepts::UniformRandomBitGenerator RNG>
        constexpr UUID operator()(RNG&& rng) const {
            auto distribution = random::UniformIntDistribution(0, 255);
            auto bytes = UUID::ByteArray {};
            for (auto& byte : bytes) {
                byte = Byte(distribution(rng));
            }

            auto result = UUID(bytes);
            result.set_version(UUID::Version::Random);
            result.set_to_standard_variant();
            return result;
        }
    };
}

constexpr inline auto generate_uuid = detail::GenerateUUIDFunction {};
}

namespace di {
inline namespace literals {
    inline namespace uuid_literals {
        consteval util::UUID operator""_uuid(char const* data, usize size) {
            auto view = di::TransparentStringView { data, size };
            return parser::parse_unchecked<util::UUID>(view);
        }
    }
}
}