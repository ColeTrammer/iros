#include <dius/prelude.h>

#include <linux/fcntl.h>
#include <linux/unistd.h>

#include <dirent.h>

namespace dius::linux {
struct Dirent {
    u64 inode;
    i64 offset;
    u16 record_length;
    u8 type;
    char name[NAME_MAX + 1];
};

static di::Expected<usize, PosixCode> sys_getdents64(int fd, void* buffer, usize nbytes) {
    return system::system_call<usize>(system::Number::getdents64, fd, buffer, nbytes);
}
}

namespace dius::filesystem {

di::Result<DirectoryIterator> DirectoryIterator::create(di::Path path, DirectoryOptions options) {
    // FIXME: handle the directory options.
    auto file_handle = TRY(open_sync(path, OpenMode::Readonly));

    auto result = DirectoryIterator(di::move(path), di::move(file_handle), options);
    ++result;
    return result;
}

void DirectoryIterator::advance_one() {
    linux::Dirent dirent;
    auto result = linux::sys_getdents64(m_directory_handle.file_descriptor(), di::addressof(dirent), sizeof(dirent));
    if (!result) {
        m_current = di::Unexpected(di::move(result).error());
    } else if (*result == 0) {
        m_at_end = true;
    } else {
        // NOTE: the FileType enum is setup to match the Linux system call ABI.
        auto type = FileType(dirent.type);
        auto path = di::clone(m_path);
        auto name = di::TransparentStringView { dirent.name,
                                                dirent.record_length - __builtin_offsetof(linux::Dirent, name) - 1 };
        path.append(name);
        m_current.emplace(DirectoryEntry(di::move(path), type));
    }
}
}