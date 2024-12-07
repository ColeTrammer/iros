#pragma once

#include "diusaudio/frame_info.h"
#include "diusaudio/sink.h"

namespace audio::iros {
auto make_iros_sink(SinkCallback callback, FrameInfo info) -> di::Result<Sink>;
}
