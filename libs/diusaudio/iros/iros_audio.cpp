#include "iros_audio.h"

#include <di/vocab/bytes/byte_buffer.h>
#include <di/vocab/expected/expected_forward_declaration.h>
#include <dius/iros/error.h>
#include <dius/iros/system_call.h>
#include <dius/print.h>
#include <diusaudio/frame.h>
#include <diusaudio/frame_info.h>
#include <diusaudio/sink.h>

namespace audio::iros {
class IrosSink {
public:
    explicit IrosSink(SinkCallback callback) : m_callback(di::move(callback)) {}

private:
    friend void tag_invoke(di::Tag<start>, IrosSink& self) {
        while (!self.m_should_stop) {
            auto buffer = di::Vector<byte> {};
            buffer.reserve(8_usize * 4096_usize);
            buffer.resize(8_usize * 4096_usize);

            auto frame = ExclusiveFrame(di::ExclusiveByteBuffer(di::move(buffer)),
                                        FrameInfo(2, SampleFormat::SignedInt16LE, 44100));

            self.m_callback(frame);

            *dius::system::system_call<usize>(dius::system::Number::write_audio, frame.as_raw_bytes().data(),
                                              frame.as_raw_bytes().size());
        }
    }

    friend void tag_invoke(di::Tag<stop>, IrosSink& self) { self.m_should_stop = true; }

    bool m_should_stop { false };
    SinkCallback m_callback;
};

auto make_iros_sink(SinkCallback callback, FrameInfo info) -> di::Result<Sink> {
    if (info.channel_count != 2 || info.format != SampleFormat::SignedInt16LE || info.sample_rate_hz != 44100) {
        return di::Unexpected(dius::PosixError::NotSupported);
    }

    return IrosSink(di::move(callback));
}
}
