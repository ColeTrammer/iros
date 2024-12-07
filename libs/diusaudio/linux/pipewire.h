#pragma once

#include <pipewire/loop.h>
#include <pipewire/main-loop.h>
#include <pipewire/pipewire.h>
#include <pipewire/stream.h>

#include "di/container/tree/tree_map.h"
#include "di/function/container/function.h"
#include "di/util/exchange.h"
#include "di/util/immovable.h"
#include "di/util/noncopyable.h"
#include "di/vocab/error/result.h"
#include "diusaudio/frame.h"
#include "diusaudio/frame_info.h"
#include "diusaudio/sink.h"

namespace audio::linux {
class PipewireLibrary : di::NonCopyable {
public:
    PipewireLibrary();
    PipewireLibrary(PipewireLibrary&& other) : m_active(di::exchange(other.m_active, false)) {}
    ~PipewireLibrary();

private:
    bool m_active { false };
};

class PipewireMainloop : di::Immovable {
public:
    PipewireMainloop();
    ~PipewireMainloop();

    void run();
    void quit();
    void register_signal_handler(u32 signo, di::Function<void()>);

    auto raw_loop() const -> pw_loop*;

private:
    pw_main_loop* m_loop { nullptr };
    di::TreeMap<u32, di::Function<void()>> m_handlers;
};

class PipewireStream : di::Immovable {
public:
    explicit PipewireStream(PipewireMainloop& loop, SinkCallback callback, FrameInfo info);
    ~PipewireStream();

    void connect();

private:
    pw_stream* m_stream { nullptr };
    SinkCallback m_sink_callback;
    FrameInfo m_info;
};

auto make_pipewire_sink(SinkCallback callback, FrameInfo info) -> di::Result<Sink>;
}
