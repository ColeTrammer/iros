#include <dius/error.h>
#include <dius/filesystem/prelude.h>

namespace dius::filesystem {
namespace iros {
    static di::Expected<int, PosixCode> create_node(di::PathView path, FileType type, Perms) {
        return system::system_call<int>(system::Number::create_node, path.data().data(), path.data().size(),
                                        static_cast<u32>(type));
    }
}

namespace detail {
    di::Result<bool> CreateRegularFileFunction::operator()(di::PathView path) const {
        auto result = iros::create_node(path, FileType::Regular,
                                        Perms::All & ~(Perms::OwnerExec | Perms::GroupExec | Perms::OwnerExec));
        if (!result) {
            if (result.error() == PosixError::FileExists) {
                return false;
            }
            return di::Unexpected(result.error());
        }
        return true;
    }

    di::Result<bool> CreateDirectoryFunction::operator()(di::PathView path) const {
        auto result = iros::create_node(path, FileType::Directory, Perms::All);
        if (!result) {
            if (result.error() == PosixError::FileExists) {
                return false;
            }
            return di::Unexpected(result.error());
        }
        return true;
    }
}
}
