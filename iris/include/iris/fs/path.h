#pragma once

#include <di/container/path/prelude.h>
#include <di/execution/any/any_sender.h>
#include <iris/core/error.h>
#include <iris/fs/tnode.h>
#include <iris/uapi/open.h>

namespace iris {
enum class PathLookupFlags {
    None = 0,
    Create = (1 << 0),
};

DI_DEFINE_ENUM_BITWISE_OPERATIONS(PathLookupFlags)

di::AnySenderOf<di::Arc<TNode>> lookup_path(di::Arc<TNode> root, di::Arc<TNode> relative_to, di::PathView path,
                                            PathLookupFlags flags = PathLookupFlags::None);

di::AnySenderOf<void> create_node(di::Arc<TNode> root, di::Arc<TNode> relative_to, di::PathView path,
                                  MetadataType type);
di::AnySenderOf<File> open_path(di::Arc<TNode> root, di::Arc<TNode> relative_to, di::PathView path, OpenMode mode);
}
