#include <diusaudio/sink.h>

#include <di/platform/prelude.h>
#include <diusaudio/frame.h>
#include <diusaudio/frame_info.h>

#ifdef DIUSAUDIO_HAVE_PIPEWIRE
#include "linux/pipewire.h"
#endif

#ifdef __iros__
#include "iros/iros_audio.h"
#endif

namespace audio {
auto make_sink([[maybe_unused]] SinkCallback callback, [[maybe_unused]] FrameInfo info) -> di::Result<Sink> {
#ifdef DIUSAUDIO_HAVE_PIPEWIRE
    return linux::make_pipewire_sink(di::move(callback), info);
#elifdef __iros__
    return iros::make_iros_sink(di::move(callback), info);
#else
    return di::Unexpected(di::BasicError::OperationNotSupported);
#endif
}
}
