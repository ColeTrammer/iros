#include "dius/error.h"
#include "dius/filesystem/prelude.h"

namespace dius::filesystem {
namespace iros {
    static auto create_node(di::PathView path, FileType type, Perms) -> di::Expected<int, di::GenericCode> {
        return system::system_call<int>(system::Number::create_node, path.data().data(), path.data().size(),
                                        static_cast<u32>(type));
    }
}

namespace detail {
    auto CreateRegularFileFunction::operator()(di::PathView path) const -> di::Result<bool> {
        auto result = iros::create_node(path, FileType::Regular,
                                        Perms::All & ~(Perms::OwnerExec | Perms::GroupExec | Perms::OthersExec));
        if (!result) {
            if (result.error() == PosixError::FileExists) {
                return false;
            }
            return di::Unexpected(result.error());
        }
        return true;
    }

    auto CreateDirectoryFunction::operator()(di::PathView path) const -> di::Result<bool> {
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
