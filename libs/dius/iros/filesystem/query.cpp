#include <di/assert/prelude.h>
#include <di/container/algorithm/prelude.h>
#include <dius/error.h>
#include <dius/filesystem/prelude.h>
#include <dius/system/system_call.h>
#include <iris/uapi/metadata.h>

namespace dius::filesystem {
namespace iros {
    static auto sys_path_metadata(di::PathView path, iris::Metadata* metadata) -> di::Expected<int, PosixError> {
        return system::system_call<int>(system::Number::path_metadata, path.data().data(), path.data().size(),
                                        metadata);
    }
}

static auto stat_to_file_status(iris::Metadata const& info) -> FileStatus {
    auto type = [&] {
        if (info.type == iris::MetadataType::Regular) {
            return FileType::Regular;
        }
        if (info.type == iris::MetadataType::Directory) {
            return FileType::Directory;
        }
        return FileType::Unknown;
    }();
    return FileStatus(type);
}

namespace detail {
    auto IsEmptyFunction::operator()(di::PathView path) const -> di::Result<bool> {
        auto info = iris::Metadata {};
        TRY(iros::sys_path_metadata(path, &info));
        if (info.type != iris::MetadataType::Regular && info.type != iris::MetadataType::Directory) {
            return di::Unexpected(PosixError::OperationNotSupported);
        }
        if (info.type == iris::MetadataType::Directory) {
            // FIXME: uncomment this when DirectoryIterator is implemented.
            // auto it = TRY(di::create<DirectoryIterator>(di::create<di::Path>(path)));
            // return it == DirectoryIterator();
        }
        return info.size == 0;
    }

    auto StatusFunction::operator()(di::PathView path) const -> di::Result<FileStatus> {
        auto info = iris::Metadata {};
        auto result = iros::sys_path_metadata(path, &info);
        if (result == di::Unexpected(PosixError::NoSuchFileOrDirectory)) {
            return FileStatus(FileType::NotFound);
        } else if (!result) {
            return di::Unexpected(di::move(result).error());
        }

        return stat_to_file_status(info);
    }

    auto SymlinkStatusFunction::operator()(di::PathView path) const -> di::Result<FileStatus> {
        auto info = iris::Metadata {};
        auto result = iros::sys_path_metadata(path, &info);
        if (result == di::Unexpected(PosixError::NoSuchFileOrDirectory)) {
            return FileStatus(FileType::NotFound);
        } else if (!result) {
            return di::Unexpected(di::move(result).error());
        }

        return stat_to_file_status(info);
    }

    auto FileSizeFunction::operator()(di::PathView path) const -> di::Result<umax> {
        auto info = iris::Metadata {};
        TRY(iros::sys_path_metadata(path, &info));

        if (info.type == iris::MetadataType::Directory) {
            return di::Unexpected(PosixError::IsADirectory);
        }
        if (info.type != iris::MetadataType::Regular) {
            return di::Unexpected(PosixError::OperationNotSupported);
        }
        return info.size;
    }

    auto HardLinkCountFunction::operator()(di::PathView) const -> di::Result<umax> {
        return di::Unexpected(PosixError::OperationNotSupported);
    }
}
}
