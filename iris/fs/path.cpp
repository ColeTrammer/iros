#include <iris/core/global_state.h>
#include <iris/fs/inode.h>
#include <iris/fs/path.h>
#include <iris/fs/tnode.h>

namespace iris {
Expected<di::Arc<TNode>> lookup_path(di::Arc<TNode> root, di::Arc<TNode> relative_to, di::PathView path) {
    auto parent = path.is_absolute() ? di::move(root) : di::move(relative_to);
    for (auto component : path) {
        // This can occur exactly once, when the path is absolute.
        if (component == "/"_tsv) {
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
        parent = TRY(inode_lookup(*inode, parent, component));
    }
    return parent;
}

Expected<File> open_path(di::Arc<TNode> root, di::Arc<TNode> relative_to, di::PathView path) {
    auto node = TRY(lookup_path(di::move(root), di::move(relative_to), path));
    return File::create(InodeFile(di::move(node)));
}
}
