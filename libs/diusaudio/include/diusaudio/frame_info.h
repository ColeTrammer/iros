#pragma once

#include "di/function/tag_invoke.h"
#include "di/reflect/enumerator.h"
#include "di/reflect/prelude.h"
#include "di/types/in_place_type.h"
#include "di/types/integers.h"
#include "di/util/unreachable.h"

namespace audio {
enum class SampleFormat {
    SignedInt16LE,
    SignedInt24LE,
    SignedInt32LE,
    Float32LE,
};

constexpr auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<SampleFormat>) {
    using enum SampleFormat;
    return di::make_enumerators(di::enumerator<"SignedInt16LE", SignedInt16LE>,
                                di::enumerator<"SignedInt24LE", SignedInt24LE>,
                                di::enumerator<"SignedInt32LE", SignedInt32LE>, di::enumerator<"Float32LE", Float32LE>);
}

constexpr auto format_bytes_per_sample(SampleFormat format) -> usize {
    using enum SampleFormat;
    switch (format) {
        case SignedInt16LE:
            return 2;
        case SignedInt24LE:
            return 3;
        case SignedInt32LE:
        case Float32LE:
            return 4;
    }
    di::unreachable();
}

struct FrameInfo {
    u32 channel_count { 1 };
    SampleFormat format { SampleFormat::Float32LE };
    u32 sample_rate_hz { 44100 };

    constexpr auto operator==(FrameInfo const&) const -> bool = default;
    constexpr auto operator<=>(FrameInfo const&) const = default;

    constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<FrameInfo>) {
        return di::make_fields(di::field<"channel_count", &FrameInfo::channel_count>,
                               di::field<"format", &FrameInfo::format>,
                               di::field<"sample_rate_hz", &FrameInfo::sample_rate_hz>);
    }
};
}
