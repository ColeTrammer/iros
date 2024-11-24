#pragma once

#include <di/assert/prelude.h>
#include <di/meta/core.h>
#include <di/meta/util.h>
#include <di/types/byte.h>
#include <di/types/floats.h>
#include <di/types/integers.h>
#include <di/util/declval.h>
#include <di/util/unreachable.h>
#include <di/vocab/bytes/byte_buffer.h>
#include <di/vocab/span/prelude.h>
#include <diusaudio/frame_info.h>

namespace audio {
namespace frame {
    template<typename Buffer>
    class FrameImpl {
    private:
        constexpr static auto is_const =
            di::SameAs<di::Span<byte const>, decltype(di::declval<Buffer const&>().span())>;

        using ConstBuffer = typename Buffer::ByteBuffer;

    public:
        FrameImpl() = default;

        constexpr explicit FrameImpl(Buffer buffer, FrameInfo info) : m_buffer(di::move(buffer)), m_info(info) {}

        constexpr auto format() const -> SampleFormat { return m_info.format; }
        constexpr auto channel_count() const -> usize { return m_info.channel_count; }
        constexpr auto sample_rate_hz() const -> usize { return m_info.sample_rate_hz; }
        constexpr auto info() const -> FrameInfo { return m_info; }

        constexpr auto sample_count() const -> usize { return as_raw_bytes().size() / stride(); }
        constexpr auto bytes_per_sample() const -> usize { return format_bytes_per_sample(format()); }
        constexpr auto stride() const -> usize { return bytes_per_sample() * channel_count(); }
        constexpr auto byte_count() const -> usize { return sample_count() * stride(); }

        auto as_float32_le() const {
            ASSERT_EQ(format(), SampleFormat::Float32LE);
            auto* data = reinterpret_cast<di::meta::MaybeConst<is_const, f32>*>(as_raw_bytes().data());
            return di::Span { data, sample_count() * channel_count() };
        }

        auto as_signed_int16_le() const {
            ASSERT_EQ(format(), SampleFormat::SignedInt16LE);
            auto* data = reinterpret_cast<di::meta::MaybeConst<is_const, i16>*>(as_raw_bytes().data());
            return di::Span { data, sample_count() * channel_count() };
        }

        auto as_signed_int32_le() const {
            ASSERT_EQ(format(), SampleFormat::SignedInt32LE);
            auto* data = reinterpret_cast<di::meta::MaybeConst<is_const, i32>*>(as_raw_bytes().data());
            return di::Span { data, sample_count() * channel_count() };
        }

        constexpr auto as_raw_bytes() const { return m_buffer.span(); }

        constexpr auto shrink_to_first_n_samples(usize count)
        requires(!is_const)
        {
            m_buffer.shrink_to_first(count * stride());
        }

    operator ConstBuffer() && requires(!is_const) { return ConstBuffer(di::move(m_buffer), m_info); }

        private : Buffer m_buffer;
        FrameInfo m_info;
    };
}

using Frame = frame::FrameImpl<di::ByteBuffer>;
using ExclusiveFrame = frame::FrameImpl<di::ExclusiveByteBuffer>;
}
