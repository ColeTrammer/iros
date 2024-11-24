#include <diusaudio/formats/wav.h>

#include <di/bit/endian/little_endian.h>
#include <di/function/monad/monad_try.h>
#include <di/io/read_all.h>
#include <di/platform/custom.h>
#include <di/util/uuid.h>
#include <di/vocab/array/array.h>
#include <di/vocab/bytes/byte_buffer.h>
#include <di/vocab/expected/expected.h>
#include <di/vocab/pointer/box.h>
#include <dius/filesystem/query/file_size.h>
#include <dius/print.h>
#include <dius/sync_file.h>
#include <diusaudio/frame_info.h>

namespace audio::formats {
auto parse_wav([[gnu::unused]] di::PathView path) -> di::Result<Frame> {
    auto file = TRY(dius::open_sync(path, dius::OpenMode::Readonly));

    // FIXME: this is an obvious race condition with opening the file.
    auto size = TRY(dius::filesystem::file_size(path));

    auto contents = di::Vector<byte> {};
    contents.reserve(size);
    contents.assume_size(size);

    TRY(file.read_exactly(contents.span()));
    auto bytes = contents.span();

    if (bytes.size() < 12) {
        return di::Unexpected(di::BasicError::InvalidArgument);
    }

    struct [[gnu::packed]] WavHeader {
        di::Array<byte, 4> chunk_id;
        di::LittleEndian<u32> chunk_size;
        di::Array<byte, 4> wave_id;
    };

    auto* header = bytes.typed_pointer_unchecked<WavHeader>(0);
    if (header->chunk_id != di::Array { 'R'_b, 'I'_b, 'F'_b, 'F'_b }) {
        return di::Unexpected(di::BasicError::InvalidArgument);
    }

    if (header->wave_id != di::Array { 'W'_b, 'A'_b, 'V'_b, 'E'_b }) {
        return di::Unexpected(di::BasicError::InvalidArgument);
    }

    if (header->chunk_size > bytes.size()) {
        return di::Unexpected(di::BasicError::InvalidArgument);
    }

    struct [[gnu::packed]] WavFmt {
        di::Array<byte, 4> chunk_id;
        di::LittleEndian<u32> chunk_size;
        di::LittleEndian<u16> format_code;
        di::LittleEndian<u16> channel_count;
        di::LittleEndian<u32> sample_rate;
        di::LittleEndian<u32> avg_bytes_per_sec;
        di::LittleEndian<u16> data_block_size;
        di::LittleEndian<u16> bits_per_sample;
    };

    auto format = bytes.typed_pointer<WavFmt>(sizeof(WavHeader));
    if (!format) {
        return di::Unexpected(di::BasicError::InvalidArgument);
    }

    if ((*format)->format_code != 1 || (*format)->bits_per_sample != 16) {
        return di::Unexpected(di::BasicError::InvalidArgument);
    }

    struct WavDataHeader {
        di::Array<byte, 4> chunk_id;
        di::LittleEndian<u32> chunk_size;
    };

    auto wav_data_header = bytes.typed_pointer<WavDataHeader>(sizeof(WavHeader) + sizeof(WavFmt));
    if (!wav_data_header) {
        return di::Unexpected(di::BasicError::InvalidArgument);
    }

    if ((*wav_data_header)->chunk_id != di::Array { 'd'_b, 'a'_b, 't'_b, 'a'_b }) {
        return di::Unexpected(di::BasicError::InvalidArgument);
    }

    auto data_offset = sizeof(WavHeader) + sizeof(WavFmt) + sizeof(WavDataHeader);
    auto data_size = (*wav_data_header)->chunk_size;
    if (data_offset + data_size > bytes.size()) {
        return di::Unexpected(di::BasicError::InvalidArgument);
    }

    auto buffer = di::ByteBuffer(di::move(contents));
    return Frame(*buffer.slice(data_offset, data_size),
                 FrameInfo((*format)->channel_count, SampleFormat::SignedInt16LE, (*format)->sample_rate));
}
}
