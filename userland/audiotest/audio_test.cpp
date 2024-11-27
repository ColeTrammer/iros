#include <di/cli/parser.h>
#include <di/container/algorithm/copy.h>
#include <di/container/algorithm/fill.h>
#include <di/container/algorithm/fill_n.h>
#include <di/container/algorithm/min.h>
#include <di/container/path/path_view.h>
#include <di/container/view/range.h>
#include <di/function/monad/monad_try.h>
#include <di/math/align_down.h>
#include <di/math/constants.h>
#include <di/math/functions.h>
#include <dius/main.h>
#include <dius/print.h>
#include <dius/system/process.h>
#include <diusaudio/formats/wav.h>
#include <diusaudio/frame.h>
#include <diusaudio/frame_info.h>
#include <diusaudio/sink.h>

namespace audiotest {
struct Args {
    di::Optional<di::PathView> wav_file;
    bool help { false };

    constexpr static auto get_cli_parser() {
        return di::cli_parser<Args>("audio_test"_sv, "Iros audio test program"_sv)
            .help()
            .option<&Args::wav_file>('w', "wave"_tsv, "Wave file to play"_sv);
    }
};

auto main(Args& args) -> di::Result<void> {
    if (args.wav_file) {
        dius::println("Trying to open WAV file: {}"_sv, *args.wav_file);
        auto result = TRY(audio::formats::parse_wav(*args.wav_file));

        dius::println("Playing WAV file with info {}, duration {}s"_sv, result.info(),
                      result.sample_count() / result.sample_rate_hz());

        auto bytes_index = 0;
        auto sink = TRY(audio::make_sink(
            [&](audio::ExclusiveFrame& frame) {
                auto output = frame.as_raw_bytes();
                auto to_copy =
                    di::align_down(di::min(output.size(), result.as_raw_bytes().size() - bytes_index), frame.stride());
                di::copy(*result.as_raw_bytes().subspan(bytes_index, to_copy), output.data());
                frame.shrink_to_first_n_samples(to_copy / frame.stride());
                bytes_index += to_copy;

                if (to_copy == 0) {
                    dius::system::exit_process(0);
                }
            },
            result.info()));

        audio::start(sink);

        return {};
    }

    dius::println("Generating sound with increasing pitch."_sv);

    auto sink = TRY(audio::make_sink(
        [](audio::ExclusiveFrame& frame) {
            static int f = 220;
            static f32 accumulator = 0;

            auto* out = frame.as_signed_int16_le().data();
            for (auto _ : di::range(frame.sample_count())) {
                accumulator += 2 * di::numbers::pi * f / frame.sample_rate_hz();
                if (accumulator >= 2 * di::numbers::pi) {
                    accumulator -= 2 * di::numbers::pi;
                }

                auto val = di::sin(accumulator) * 0.5;
                auto int_val = i16(val * di::NumericLimits<i16>::max);
                out = di::fill_n(out, frame.channel_count(), int_val);
            }
            f++;
            if (f == 880) {
                f = 220;
            }
        },
        audio::FrameInfo(2, audio::SampleFormat::SignedInt16LE)));

    audio::start(sink);

    return {};
}
}

DIUS_MAIN(audiotest::Args, audiotest)
