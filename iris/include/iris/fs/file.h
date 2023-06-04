#pragma once

#include <di/any/prelude.h>
#include <di/bit/bitset/prelude.h>
#include <di/execution/algorithm/just.h>
#include <di/execution/any/any_sender.h>
#include <di/execution/receiver/prelude.h>
#include <di/execution/types/prelude.h>
#include <di/types/prelude.h>
#include <iris/core/error.h>
#include <iris/core/userspace_buffer.h>
#include <iris/uapi/metadata.h>

namespace iris {
namespace detail {
    struct WriteFileDefaultFunction {
        template<typename T>
        di::AnySenderOf<usize> operator()(T&, UserspaceBuffer<byte const>) const {
            return di::Unexpected(Error::NotSupported);
        }
    };

    struct ReadFileDefaultFunction {
        template<typename T>
        di::AnySenderOf<usize> operator()(T&, UserspaceBuffer<byte>) const {
            return di::Unexpected(Error::NotSupported);
        }
    };

    struct ReadDirectoryDefaultFunction {
        template<typename T>
        di::AnySenderOf<usize> operator()(T&, UserspaceBuffer<byte>) const {
            return di::Unexpected(Error::NotSupported);
        }
    };

    struct SeekFileDefaultFunction {
        di::AnySenderOf<i64> operator()(auto&, i64, int) const { return di::Unexpected(Error::NotSupported); }
    };

    struct FileMetadataDefaultFunction {
        di::AnySenderOf<Metadata> operator()(auto&) const { return di::Unexpected(Error::NotSupported); }
    };

    struct FileTruncateDefaultFunction {
        di::AnySenderOf<> operator()(auto&, u64) const { return di::Unexpected(Error::NotSupported); }
    };

    struct FileHACKRawDataFunction {
        di::AnySenderOf<di::Span<byte const>> operator()(auto&) const { return di::Unexpected(Error::NotSupported); }
    };
}

struct WriteFileFunction
    : di::Dispatcher<WriteFileFunction, di::AnySenderOf<usize>(di::This&, UserspaceBuffer<byte const>),
                     detail::WriteFileDefaultFunction> {};

constexpr inline auto write_file = WriteFileFunction {};

struct ReadFileFunction
    : di::Dispatcher<ReadFileFunction, di::AnySenderOf<usize>(di::This&, UserspaceBuffer<byte>),
                     detail::ReadFileDefaultFunction> {};

constexpr inline auto read_file = ReadFileFunction {};

struct ReadDirectoryFunction
    : di::Dispatcher<ReadDirectoryFunction, di::AnySenderOf<usize>(di::This&, UserspaceBuffer<byte>),
                     detail::ReadDirectoryDefaultFunction> {};

constexpr inline auto read_directory = ReadDirectoryFunction {};

struct FileMetadataFunction
    : di::Dispatcher<FileMetadataFunction, di::AnySenderOf<Metadata>(di::This&), detail::FileMetadataDefaultFunction> {
};

constexpr inline auto file_metadata = FileMetadataFunction {};

struct SeekFileFunction
    : di::Dispatcher<SeekFileFunction, di::AnySenderOf<i64>(di::This&, i64, int), detail::SeekFileDefaultFunction> {};

constexpr inline auto seek_file = SeekFileFunction {};

struct FileTruncateFunction
    : di::Dispatcher<FileTruncateFunction, di::AnySenderOf<>(di::This&, u64), detail::FileTruncateDefaultFunction> {};

constexpr inline auto file_truncate = FileTruncateFunction {};

struct FileHACKRawDataFunction
    : di::Dispatcher<FileHACKRawDataFunction, di::AnySenderOf<di::Span<byte const>>(di::This&),
                     detail::FileHACKRawDataFunction> {};

constexpr inline auto file_hack_raw_data = FileHACKRawDataFunction {};

using FileInterface = di::meta::List<WriteFileFunction, ReadFileFunction, ReadDirectoryFunction, FileMetadataFunction,
                                     SeekFileFunction, FileTruncateFunction, FileHACKRawDataFunction>;
using File = di::AnyShared<FileInterface>;

class FileTable {
public:
    Expected<di::Tuple<File&, i32>> allocate_file_handle() {
        for (auto [i, file] : di::enumerate(m_files)) {
            if (!file.has_value()) {
                m_file_allocated[i] = true;
                return di::make_tuple(di::ref(file), static_cast<i32>(i));
            }
        }
        return di::Unexpected(Error::TooManyFilesOpen);
    }

    Expected<File&> lookup_file_handle(i32 file_handle) {
        if (file_handle < 0 || di::equal_or_greater(file_handle, m_files.size())) {
            return di::Unexpected(Error::BadFileDescriptor);
        }
        if (!m_file_allocated[file_handle] || !m_files[file_handle].has_value()) {
            return di::Unexpected(Error::BadFileDescriptor);
        }
        return m_files[file_handle];
    }

    Expected<void> deallocate_file_handle(i32 file_handle) {
        if (file_handle < 0 || di::equal_or_greater(file_handle, m_files.size())) {
            return di::Unexpected(Error::BadFileDescriptor);
        }
        if (!m_file_allocated[file_handle] || !m_files[file_handle].has_value()) {
            return di::Unexpected(Error::BadFileDescriptor);
        }
        m_files[file_handle].reset();
        m_file_allocated[file_handle] = false;
        return {};
    }

private:
    di::Array<File, 16> m_files {};
    di::BitSet<16> m_file_allocated;
};
}
