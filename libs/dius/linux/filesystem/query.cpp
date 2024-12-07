#include "di/assert/prelude.h"
#include "di/container/algorithm/prelude.h"
#include "dius/filesystem/prelude.h"
#include "dius/system/system_call.h"

#ifdef DIUS_USE_RUNTIME
#include <linux/fcntl.h>
#include <linux/stat.h>
#include <linux/time.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#endif

namespace dius::filesystem {
namespace linux {
    struct stat64 {
        u64 device;
        u64 inode;
        u64 link_count;
        u32 mode;
        u32 uid;
        u32 gid;
        di::Array<di::Byte, 4> pading;
        u64 device_number;
        i64 size;
        i64 block_size;
        i64 block_count;
        timespec access_time;
        timespec modified_time;
        timespec creation_time;
        di::Array<di::Byte, 24> padding_end;
    };

    static auto sys_fstatat64(int dirfd, di::PathView path, stat64* buffer, int flags) -> di::Result<int> {
        auto raw_data = path.data();
        char null_terminated_string[4097];
        ASSERT_LT(raw_data.size(), sizeof(null_terminated_string) - 1);

        di::copy(raw_data, null_terminated_string);
        null_terminated_string[raw_data.size()] = '\0';

        return system::system_call<int>(system::Number::fstatat64, dirfd, null_terminated_string, buffer, flags);
    }
}

static auto stat_to_file_status(linux::stat64& info) -> FileStatus {
    auto type = [&] {
        if (S_ISREG(info.mode)) {
            return FileType::Regular;
        }
        if (S_ISDIR(info.mode)) {
            return FileType::Directory;
        }
        if (S_ISBLK(info.mode)) {
            return FileType::Block;
        }
        if (S_ISCHR(info.mode)) {
            return FileType::Character;
        }
        if (S_ISFIFO(info.mode)) {
            return FileType::Fifo;
        }
        if (S_ISSOCK(info.mode)) {
            return FileType::Socket;
        }
        if (S_ISLNK(info.mode)) {
            return FileType::Symlink;
        }
        return FileType::Unknown;
    }();
    return FileStatus(type, Perms(info.mode) & Perms::Mask);
}

namespace detail {
    auto IsEmptyFunction::operator()(di::PathView path) const -> di::Result<bool> {
        linux::stat64 info;
        auto result = linux::sys_fstatat64(AT_FDCWD, path, &info, 0);
        if (!S_ISDIR(info.mode) && !S_ISREG(info.mode)) {
            return di::Unexpected(PosixError::OperationNotSupported);
        }
        if (S_ISDIR(info.mode)) {
            auto it = TRY(di::create<DirectoryIterator>(di::create<di::Path>(path)));
            return it == DirectoryIterator();
        }
        return info.size == 0;
    }

    auto StatusFunction::operator()(di::PathView path) const -> di::Result<FileStatus> {
        linux::stat64 info;
        auto result = linux::sys_fstatat64(AT_FDCWD, path, &info, 0);
        if (result == di::Unexpected(PosixError::NoSuchFileOrDirectory)) {
            return FileStatus(FileType::NotFound);
        }
        if (!result) {
            return di::Unexpected(di::move(result).error());
        }

        return stat_to_file_status(info);
    }

    auto SymlinkStatusFunction::operator()(di::PathView path) const -> di::Result<FileStatus> {
        linux::stat64 info;
        auto result = linux::sys_fstatat64(AT_FDCWD, path, &info, AT_SYMLINK_NOFOLLOW);
        if (result == di::Unexpected(PosixError::NoSuchFileOrDirectory)) {
            return FileStatus(FileType::NotFound);
        }
        if (!result) {
            return di::Unexpected(di::move(result).error());
        }

        return stat_to_file_status(info);
    }

    auto FileSizeFunction::operator()(di::PathView path) const -> di::Result<umax> {
        linux::stat64 info;
        TRY(linux::sys_fstatat64(AT_FDCWD, path, &info, 0));

        if (S_ISDIR(info.mode)) {
            return di::Unexpected(PosixError::IsADirectory);
        }
        if (!S_ISREG(info.mode)) {
            return di::Unexpected(PosixError::OperationNotSupported);
        }
        return info.size;
    }

    auto HardLinkCountFunction::operator()(di::PathView path) const -> di::Result<umax> {
        linux::stat64 info;
        TRY(linux::sys_fstatat64(AT_FDCWD, path, &info, 0));
        return info.link_count;
    }
}
}
