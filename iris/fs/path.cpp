#include <di/execution/algorithm/into_result.h>
#include <di/execution/any/any_sender.h>
#include <di/util/prelude.h>
#include <iris/core/global_state.h>
#include <iris/core/print.h>
#include <iris/fs/inode.h>
#include <iris/fs/path.h>
#include <iris/fs/tnode.h>
#include <iris/uapi/metadata.h>

namespace iris {
di::AnySenderOf<di::Arc<TNode>> lookup_path(di::Arc<TNode> root, di::Arc<TNode> relative_to, di::PathView path,
                                            PathLookupFlags flags) {
    auto parent = path.is_absolute() ? di::move(root) : di::move(relative_to);
    for (auto it = path.begin(); it != path.end(); ++it) {
        auto component = *it;

        // This can occur exactly once, when the path is absolute.
        if (component == "/"_tsv) {
            continue;
        }

        if (component == "."_tsv) {
            continue;
        }

        if (component == ".."_tsv) {
            // If the current node is the root, continue without fetching the parent (which is null).
            if (!parent->parent()) {
                continue;
            }
            parent = parent->parent();
            continue;
        }

        auto inode = parent->inode();

        auto result = co_await di::execution::into_result(inode_lookup(*inode, parent, component));
        if (result == di::Unexpected(Error::NoSuchFileOrDirectory) && !!(flags & PathLookupFlags::Create)) {
            // Now try to create the file, but only if this is the last component in the path.
            if (di::next(it) != path.end()) {
                co_await di::execution::just_error(Error::NoSuchFileOrDirectory);
                di::unreachable();
            }

            co_return co_await inode_create_node(*inode, parent, component, MetadataType::Regular);
        }

        parent = co_await di::move(result);

        // See if there is an existing mount.
        inode = parent->inode();
        if (auto mount = inode->mount(); mount) {
            auto parent_inode = mount->super_block().root_inode();
            parent =
                co_await di::make_arc<TNode>(di::move(parent), di::move(parent_inode), co_await component.to_owned());
            continue;
        }
    }
    co_return parent;
}

di::AnySenderOf<void> create_node(di::Arc<TNode> root, di::Arc<TNode> relative_to, di::PathView path,
                                  MetadataType type) {
    auto parent_path = path.parent_path();
    if (!parent_path) {
        co_await di::execution::just_error(Error::InvalidArgument);
        di::unreachable();
    }

    auto parent = co_await lookup_path(di::move(root), di::move(relative_to), *parent_path);
    auto component = *path.back();

    auto result = co_await di::execution::into_result(inode_lookup(*parent->inode(), parent, component));
    if (result.has_value()) {
        co_await di::execution::just_error(Error::FileExists);
        di::unreachable();
    }

    co_await inode_create_node(*parent->inode(), parent, component, type);
}

di::AnySenderOf<File> open_path(di::Arc<TNode> root, di::Arc<TNode> relative_to, di::PathView path, OpenMode mode) {
    auto flags = !!(mode & OpenMode::Create) ? PathLookupFlags::Create : PathLookupFlags::None;
    auto node = co_await lookup_path(di::move(root), di::move(relative_to), path, flags);
    co_return co_await File::create(InodeFile(di::move(node)));
}
}
