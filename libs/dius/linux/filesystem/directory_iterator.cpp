#include <di/assert/prelude.h>
#include <di/math/prelude.h>
#include <dius/filesystem/prelude.h>
#include <dius/system/system_call.h>

#ifdef DIUS_USE_RUNTIME
#include <linux/fcntl.h>
#include <linux/unistd.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace dius::linux {
struct [[gnu::packed]] Dirent {
    u64 inode;
    i64 offset;
    u16 record_length;
    u8 type;

    char const* name() const { return reinterpret_cast<char const*>(this + 1); }
};

static di::Expected<usize, di::GenericCode> sys_getdents64(int fd, void* buffer, usize nbytes) {
    return system::system_call<usize>(system::Number::getdents64, fd, buffer, nbytes);
}
}

namespace dius::filesystem {

di::Expected<DirectoryIterator, di::GenericCode> DirectoryIterator::create(di::Path path, DirectoryOptions options) {
    // FIXME: handle the directory options.
    (void) options;
    auto file_handle = TRY(open_sync(path, OpenMode::Readonly));

    auto buffer = di::Vector<di::Byte> {};
    buffer.reserve(4096);

    auto result = DirectoryIterator(di::move(path), di::move(buffer), di::move(file_handle));
    ++result;

    return result;
}

void DirectoryIterator::advance_one() {
    // If the result is an actual error object, simply advance to the end.
    if (!m_current && m_current != di::Unexpected(PosixError::Success)) {
        m_at_end = true;
        return;
    }

    for (;;) {
        advance();

        if (m_at_end || !m_current) {
            break;
        }

        if (m_current->path_view().filename() == "."_tsv || m_current->path_view().filename() == ".."_tsv) {
            continue;
        }
        break;
    }
}

void DirectoryIterator::advance() {
    // Re-fill the buffer of directory entries.
    if (m_buffer.size() == 0) {
        auto result = linux::sys_getdents64(m_directory_handle.file_descriptor(), m_buffer.data(), m_buffer.capacity());
        if (!result) {
            m_current = di::Unexpected(result.error());
            return;
        }

        if (*result == 0) {
            m_at_end = true;
            return;
        }

        m_buffer.assume_size(*result);
    }

    auto* dirent = m_buffer.span().typed_pointer_unchecked<linux::Dirent const>(m_current_offset);
    if (dirent->record_length == 0) {
        m_buffer.clear();
        return advance_one();
    }

    ASSERT_GT_EQ(dirent->record_length, sizeof(*dirent));
    m_current_offset += dirent->record_length;

    // NOTE: the FileType enum is setup to match the Linux system call ABI.
    auto type = FileType(dirent->type);
    auto path = di::clone(m_path);
    auto name = di::ZCString(dirent->name());
    path.append(di::TransparentStringView { name.data(), di::to_unsigned(di::distance(name)) });
    m_current.emplace(DirectoryEntry(di::move(path), type));

    // If we are now at the end, clear the buffer.
    if (m_current_offset >= m_buffer.size()) {
        m_buffer.clear();
        m_current_offset = 0;
    }
}
}
