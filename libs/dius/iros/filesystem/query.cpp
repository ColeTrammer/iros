#include <di/assert/prelude.h>
#include <di/container/algorithm/prelude.h>
#include <dius/error.h>
#include <dius/filesystem/prelude.h>
#include <dius/system/system_call.h>
#include <iris/uapi/metadata.h>

namespace dius::filesystem {
namespace iros {
    static di::Expected<int, PosixError> sys_path_metadata(di::PathView path, iris::Metadata* metadata) {
        return system::system_call<int>(system::Number::path_metadata, path.data().data(), path.data().size(),
                                        metadata);
    }
}

static FileStatus stat_to_file_status(iris::Metadata const& info) {
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
    di::Result<bool> IsEmptyFunction::operator()(di::PathView path) const {
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

    di::Result<FileStatus> StatusFunction::operator()(di::PathView path) const {
        auto info = iris::Metadata {};
        auto result = iros::sys_path_metadata(path, &info);
        if (result == di::Unexpected(PosixError::NoSuchFileOrDirectory)) {
            return FileStatus(FileType::NotFound);
        } else if (!result) {
            return di::Unexpected(di::move(result).error());
        }

        return stat_to_file_status(info);
    }

    di::Result<FileStatus> SymlinkStatusFunction::operator()(di::PathView path) const {
        auto info = iris::Metadata {};
        auto result = iros::sys_path_metadata(path, &info);
        if (result == di::Unexpected(PosixError::NoSuchFileOrDirectory)) {
            return FileStatus(FileType::NotFound);
        } else if (!result) {
            return di::Unexpected(di::move(result).error());
        }

        return stat_to_file_status(info);
    }

    di::Result<umax> FileSizeFunction::operator()(di::PathView path) const {
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

    di::Result<umax> HardLinkCountFunction::operator()(di::PathView) const {
        return di::Unexpected(PosixError::OperationNotSupported);
    }
}
}
