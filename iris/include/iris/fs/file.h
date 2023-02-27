#pragma once

#include <di/prelude.h>

#include <iris/core/error.h>

namespace iris {
namespace detail {
    struct WriteFileDefaultFunction {
        template<typename T>
        constexpr Expected<usize> operator()(T&, di::Span<di::Byte const>) const {
            return di::Unexpected(Error::NotSupported);
        }
    };
}

struct WriteFileFunction
    : di::Dispatcher<WriteFileFunction, Expected<usize>(di::This&, di::Span<di::Byte const>),
                     detail::WriteFileDefaultFunction> {};

constexpr inline auto write_file = WriteFileFunction {};

using FileInterface = di::meta::List<WriteFileFunction>;
using File = di::AnyShared<FileInterface>;

class FileTable {
public:
    Expected<File&> allocate_file_handle() {
        for (auto [i, file] : di::enumerate(m_files)) {
            if (file.empty()) {
                // Set file to a dummy value so that it cannot be allocated
                // again. The caller is expected to assign a proper value for
                // this file descriptor.
                m_file_allocated[i] = true;
                return file;
            }
        }
        return di::Unexpected(Error::TooManyFilesOpen);
    }

    Expected<File&> lookup_file_handle(i32 file_handle) {
        if (file_handle < 0 || di::equal_or_greater(file_handle, m_files.size())) {
            return di::Unexpected(Error::BadFileDescriptor);
        }
        if (!m_file_allocated[file_handle] || m_files[file_handle].empty()) {
            return di::Unexpected(Error::BadFileDescriptor);
        }
        return m_files[file_handle];
    }

    Expected<void> deallocate_file_handle(i32 file_handle) {
        if (file_handle < 0 || di::equal_or_greater(file_handle, m_files.size())) {
            return di::Unexpected(Error::BadFileDescriptor);
        }
        if (!m_file_allocated[file_handle] || m_files[file_handle].empty()) {
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
