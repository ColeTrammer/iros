#include "di/container/string/string_view.h"
#include "di/format/to_string.h"
#include "di/random/distribution/uniform_int_distribution.h"
#include "di/serialization/base64.h"
#include "di/vocab/array/array.h"
#include "dius/test/prelude.h"

namespace serialization_base64 {
struct Case {
    di::Vector<byte> bytes;
    di::StringView base64;
};

constexpr auto cases = []() {
    return di::Array {
        Case({}, ""_sv),
        Case({ 'f'_b }, "Zg=="_sv),
        Case({ 'f'_b, 'o'_b }, "Zm8="_sv),
        Case({ 'f'_b, 'o'_b, 'o'_b }, "Zm9v"_sv),
        Case({ 'f'_b, 'o'_b, 'o'_b, 'b'_b }, "Zm9vYg=="_sv),
        Case({ 'f'_b, 'o'_b, 'o'_b, 'b'_b, 'a'_b }, "Zm9vYmE="_sv),
        Case({ 'f'_b, 'o'_b, 'o'_b, 'b'_b, 'a'_b, 'r'_b }, "Zm9vYmFy"_sv),
    };
};

constexpr void base64_encode() {
    for (auto const& [input, output] : cases()) {
        auto result = di::to_string(di::Base64View(input.span()));
        ASSERT_EQ(result, output);
    }
}

constexpr void base64_decode() {
    for (auto const& [output, input] : cases()) {
        auto result = di::parse_unchecked<di::Base64<>>(input);
        ASSERT_EQ(result.container(), output);
    }
}

constexpr void base64_errors() {
    ASSERT(!di::parse<di::Base64<>>("A==="_sv));
    ASSERT(!di::parse<di::Base64<>>("===="_sv));
    ASSERT(!di::parse<di::Base64<>>("AAAA===="_sv));
    ASSERT(!di::parse<di::Base64<>>("AA==BBBB"_sv));
    ASSERT(!di::parse<di::Base64<>>("AAAA-BBB"_sv));
    ASSERT(!di::parse<di::Base64<>>("AAAABBB"_sv));
    ASSERT(!di::parse<di::Base64<>>("AAAABB"_sv));
    ASSERT(!di::parse<di::Base64<>>("AAAAB"_sv));
}

constexpr void base64_literal() {
    auto base64 = "ASDF"_base64;
    ASSERT_EQ(di::to_string(base64), "ASDF"_sv);
}

constexpr void base64_roundtrip() {
    auto do_test = [](di::UniformRandomBitGenerator auto rng) {
        auto n = di::UniformIntDistribution(0, 100)(rng);
        auto bytes = di::Vector<byte> {};
        for (auto _ : di::range(n)) {
            bytes.push_back(byte(di::UniformIntDistribution(0, 255)(rng)));
        }

        auto base64 = di::Base64(di::move(bytes));
        auto string = di::to_string(base64);
        auto result = di::parse_unchecked<di::Base64<>>(string);
        ASSERT_EQ(base64, result);
    };

    do_test(di::MinstdRand(1));

    if (!di::is_constant_evaluated()) {
        do_test(di::MinstdRand(2));
        do_test(di::MinstdRand(3));
        do_test(di::MinstdRand(4));
        do_test(di::MinstdRand(5));
        do_test(di::MinstdRand(6));
        do_test(di::MinstdRand(7));
        do_test(di::MinstdRand(8));
        do_test(di::MinstdRand(9));
        do_test(di::MinstdRand(10));
    }
}

TESTC(serialization_base64, base64_encode)
TESTC(serialization_base64, base64_decode)
TESTC(serialization_base64, base64_errors)
TESTC(serialization_base64, base64_literal)
TESTC(serialization_base64, base64_roundtrip)
}
