#pragma once

#include <di/any/dispatch/prelude.h>
#include <di/any/prelude.h>
#include <di/container/tree/prelude.h>
#include <di/execution/any/any_sender.h>
#include <di/types/integers.h>
#include <di/vocab/optional/prelude.h>
#include <iris/core/error.h>
#include <iris/fs/file.h>
#include <iris/fs/mount.h>
#include <iris/mm/backing_object.h>
#include <iris/mm/physical_address.h>
#include <iris/uapi/metadata.h>

namespace iris {
class TNode;

struct InodeReadFunction
    : di::Dispatcher<InodeReadFunction,
                     di::AnySenderOf<mm::PhysicalAddress>(di::This&, mm::BackingObject&, u64 page_number)> {};

constexpr inline auto inode_read = InodeReadFunction {};

struct InodeReadDirectoryFunction
    : di::Dispatcher<InodeReadDirectoryFunction,
                     di::AnySenderOf<usize>(di::This&, mm::BackingObject&, u64& offset, UserspaceBuffer<byte> buffer)> {
};

constexpr inline auto inode_read_directory = InodeReadDirectoryFunction {};

struct InodeLookupFunction
    : di::Dispatcher<InodeLookupFunction,
                     di::AnySenderOf<di::Arc<TNode>>(di::This&, di::Arc<TNode>, di::TransparentStringView)> {};

constexpr inline auto inode_lookup = InodeLookupFunction {};

struct InodeMetadataFunction : di::Dispatcher<InodeMetadataFunction, di::AnySenderOf<Metadata>(di::This&)> {};

constexpr inline auto inode_metadata = InodeMetadataFunction {};

struct InodeCreateNodeFunction
    : di::Dispatcher<InodeCreateNodeFunction, di::AnySenderOf<di::Arc<TNode>>(
                                                  di::This&, di::Arc<TNode>, di::TransparentStringView, MetadataType)> {
};

constexpr inline auto inode_create_node = InodeCreateNodeFunction {};

struct InodeTruncateFunction : di::Dispatcher<InodeTruncateFunction, di::AnySenderOf<>(di::This&, u64)> {};

constexpr inline auto inode_truncate = InodeTruncateFunction {};

struct InodeHACKRawDataFunction
    : di::Dispatcher<InodeHACKRawDataFunction, di::AnySenderOf<di::Span<byte const>>(di::This&)> {};

constexpr inline auto inode_hack_raw_data = InodeHACKRawDataFunction {};

using InodeInterface =
    di::meta::List<InodeReadFunction, InodeReadDirectoryFunction, InodeLookupFunction, InodeMetadataFunction,
                   InodeCreateNodeFunction, InodeTruncateFunction, InodeHACKRawDataFunction>;

using InodeImpl = di::Any<InodeInterface>;

class InodeFile {
public:
    explicit InodeFile(di::Arc<TNode> tnode);

    friend auto tag_invoke(di::Tag<read_file>, InodeFile& self, UserspaceBuffer<byte> buffer) -> di::AnySenderOf<usize>;
    friend auto tag_invoke(di::Tag<read_directory>, InodeFile& self, UserspaceBuffer<byte> buffer)
        -> di::AnySenderOf<usize>;
    friend auto tag_invoke(di::Tag<write_file>, InodeFile& self, UserspaceBuffer<byte const> buffer)
        -> di::AnySenderOf<usize>;
    friend auto tag_invoke(di::Tag<file_metadata>, InodeFile& self) -> di::AnySenderOf<Metadata>;
    friend auto tag_invoke(di::Tag<seek_file>, InodeFile& self, i64 offset, int whence) -> di::AnySenderOf<u64>;
    friend auto tag_invoke(di::Tag<file_truncate>, InodeFile& self, u64 size) -> di::AnySenderOf<>;
    friend auto tag_invoke(di::Tag<file_hack_raw_data>, InodeFile& self) -> di::AnySenderOf<di::Span<byte const>>;

private:
    di::Arc<TNode> m_tnode;
    u64 m_offset { 0 };
};

class Inode : public di::IntrusiveRefCount<Inode> {
public:
    explicit Inode(InodeImpl impl) : m_impl(di::move(impl)) {}

    auto backing_object() -> mm::BackingObject& { return m_backing_object; }

    auto mount() const -> di::Optional<Mount&> { return m_mount.transform(di::chain(di::dereference, di::ref)); }
    void set_mount(di::Box<Mount> mount) { m_mount = di::move(mount); }

private:
    friend auto tag_invoke(di::Tag<inode_read>, Inode& self, mm::BackingObject& backing_object, u64 page_number)
        -> di::AnySenderOf<mm::PhysicalAddress>;
    friend auto tag_invoke(di::Tag<inode_read_directory>, Inode& self, mm::BackingObject& backing_object, u64& offset,
                           UserspaceBuffer<byte> buffer) -> di::AnySenderOf<usize>;
    friend auto tag_invoke(di::Tag<inode_lookup>, Inode& self, di::Arc<TNode> parent, di::TransparentStringView name)
        -> di::AnySenderOf<di::Arc<TNode>>;
    friend auto tag_invoke(di::Tag<inode_metadata>, Inode& self) -> di::AnySenderOf<Metadata>;
    friend auto tag_invoke(di::Tag<inode_create_node>, Inode& self, di::Arc<TNode> parent,
                           di::TransparentStringView name, MetadataType type) -> di::AnySenderOf<di::Arc<TNode>>;
    friend auto tag_invoke(di::Tag<inode_truncate>, Inode& self, u64 size) -> di::AnySenderOf<>;
    friend auto tag_invoke(di::Tag<inode_hack_raw_data>, Inode& self) -> di::AnySenderOf<di::Span<byte const>>;

    InodeImpl m_impl;
    di::Optional<di::Box<Mount>> m_mount;
    mm::BackingObject m_backing_object;
};
}
