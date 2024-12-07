#ifdef DIUSAUDIO_HAVE_PIPEWIRE
#include "pipewire.h"

#include <pipewire/core.h>
#include <pipewire/main-loop.h>
#include <pipewire/pipewire.h>
#include <pipewire/port.h>
#include <pipewire/properties.h>
#include <pipewire/stream.h>
#include <spa/param/audio/format-utils.h>
#include <spa/param/audio/raw-utils.h>
#include <spa/param/audio/raw.h>
#include <spa/param/param.h>

#include "di/assert/assert_bool.h"
#include "di/container/algorithm/min.h"
#include "di/function/container/function.h"
#include "di/types/integers.h"
#include "di/util/as_const_pointer.h"
#include "di/util/exchange.h"
#include "di/util/unreachable.h"
#include "di/vocab/bytes/byte_buffer.h"
#include "di/vocab/pointer/box.h"
#include "dius/print.h"
#include "diusaudio/frame.h"
#include "diusaudio/frame_info.h"
#include "diusaudio/sink.h"

namespace audio::linux {
PipewireLibrary::PipewireLibrary() : m_active(true) {
    pw_init(nullptr, nullptr);
}

PipewireLibrary::~PipewireLibrary() {
    if (m_active) {
        pw_deinit();
    }
}

PipewireMainloop::PipewireMainloop() {
    m_loop = pw_main_loop_new(nullptr);
}

PipewireMainloop::~PipewireMainloop() {
    if (auto* loop = di::exchange(m_loop, nullptr)) {
        pw_main_loop_destroy(loop);
    }
}

void PipewireMainloop::run() {
    DI_ASSERT(m_loop);
    pw_main_loop_run(m_loop);
}

void PipewireMainloop::quit() {
    DI_ASSERT(m_loop);
    pw_main_loop_quit(m_loop);
}

void PipewireMainloop::register_signal_handler(u32 signo, di::Function<void()> f) {
    DI_ASSERT(m_loop);
    m_handlers[signo] = di::move(f);

    __extension__ pw_loop_add_signal(
        raw_loop(), (int) signo,
        [](void* self, int signo) {
            static_cast<PipewireMainloop*>(self)->m_handlers[u32(signo)]();
        },
        this);
}

auto PipewireMainloop::raw_loop() const -> pw_loop* {
    DI_ASSERT(m_loop);
    return pw_main_loop_get_loop(m_loop);
}

PipewireStream::PipewireStream(PipewireMainloop& loop, SinkCallback callback, FrameInfo info)
    : m_sink_callback(di::move(callback)), m_info(info) {
    auto* props = pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio", PW_KEY_MEDIA_CATEGORY, "Playback", PW_KEY_MEDIA_ROLE,
                                    "Music", NULL);

    static auto events = pw_stream_events {};
    events.version = PW_VERSION_STREAM_EVENTS;
    events.process = [](void* closure) {
        auto* self = static_cast<PipewireStream*>(closure);
        DI_ASSERT(self->m_stream);

        auto* buffer = pw_stream_dequeue_buffer(self->m_stream);
        if (!buffer) {
            return;
        }

        auto& data = buffer->buffer->datas[0];

        auto byte_count =
            di::min(buffer->requested * self->m_info.channel_count * format_bytes_per_sample(self->m_info.format),
                    usize(data.maxsize));
        auto* byte_data = static_cast<byte*>(data.data);

        // NOTE: this is safe because pipewire manages the memory for us.
        auto frame = ExclusiveFrame(di::ExclusiveByteBuffer(di::Span { byte_data, byte_count }), self->m_info);
        self->m_sink_callback(frame);

        data.chunk->offset = 0;
        data.chunk->stride = (i32) frame.stride();
        data.chunk->size = frame.byte_count();

        pw_stream_queue_buffer(self->m_stream, buffer);
    };

    m_stream = pw_stream_new_simple(loop.raw_loop(), "audio-src-2", props, &events, this);
}

PipewireStream::~PipewireStream() {
    if (auto* stream = di::exchange(m_stream, nullptr)) {
        pw_stream_destroy(stream);
    }
}

void PipewireStream::connect() {
    DI_ASSERT(m_stream);

    auto audio_init = spa_audio_info_raw {};
    audio_init.rate = m_info.sample_rate_hz;
    audio_init.channels = m_info.channel_count;
    audio_init.format = [&] {
        using enum SampleFormat;
        switch (m_info.format) {
            case SignedInt16LE:
                return SPA_AUDIO_FORMAT_S16_LE;
            case SignedInt24LE:
                return SPA_AUDIO_FORMAT_S24_LE;
            case SignedInt32LE:
                return SPA_AUDIO_FORMAT_S32_LE;
            case Float32LE:
                return SPA_AUDIO_FORMAT_F32_LE;
        }

        di::unreachable();
    }();

    auto builder_buffer = di::Array<u8, 1024> {};
    auto builder = spa_pod_builder {};
    builder.data = builder_buffer.data();
    builder.size = builder_buffer.size();
    auto params =
        di::Array { di::as_const_pointer(spa_format_audio_raw_build(&builder, SPA_PARAM_EnumFormat, &audio_init)) };

    auto result = pw_stream_connect(
        m_stream, PW_DIRECTION_OUTPUT, PW_ID_ANY,
        pw_stream_flags(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS | PW_STREAM_FLAG_RT_PROCESS),
        params.data(), u32(params.size()));

    DI_ASSERT_EQ(result, 0);
}

class PipewireSink {
public:
    PipewireSink(SinkCallback callback, FrameInfo info)
        : m_loop(di::make_box<PipewireMainloop>())
        , m_stream(di::make_box<PipewireStream>(*m_loop, di::move(callback), info)) {}

    friend void tag_invoke(di::Tag<start>, PipewireSink& self) {
        self.m_stream->connect();
        self.m_loop->run();
    }

    friend void tag_invoke(di::Tag<stop>, PipewireSink& self) { self.m_loop->quit(); }

private:
    PipewireLibrary m_library;
    di::Box<PipewireMainloop> m_loop;
    di::Box<PipewireStream> m_stream;
};

auto make_pipewire_sink(SinkCallback callback, FrameInfo info) -> di::Result<Sink> {
    return PipewireSink(di::move(callback), info);
}
}
#endif
