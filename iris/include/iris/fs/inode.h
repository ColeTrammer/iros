#pragma once

#include <di/any/dispatch/prelude.h>
#include <di/any/prelude.h>
#include <di/container/tree/prelude.h>
#include <di/types/integers.h>
#include <iris/core/error.h>
#include <iris/fs/file.h>
#include <iris/fs/mount.h>
#include <iris/mm/backing_object.h>
#include <iris/mm/physical_address.h>
#include <iris/uapi/metadata.h>

namespace iris {
struct TNode;

struct InodeReadFunction
    : di::Dispatcher<InodeReadFunction,
                     di::AnySenderOf<mm::PhysicalAddress>(di::This&, mm::BackingObject&, u64 page_number)> {};

constexpr inline auto inode_read = InodeReadFunction {};

struct InodeReadDirectoryFunction
    : di::Dispatcher<InodeReadDirectoryFunction,
                     Expected<usize>(di::This&, mm::BackingObject&, u64& offset, UserspaceBuffer<byte> buffer)> {};

constexpr inline auto inode_read_directory = InodeReadDirectoryFunction {};

struct InodeLookupFunction
    : di::Dispatcher<InodeLookupFunction,
                     Expected<di::Arc<TNode>>(di::This&, di::Arc<TNode>, di::TransparentStringView)> {};

constexpr inline auto inode_lookup = InodeLookupFunction {};

struct InodeMetadataFunction : di::Dispatcher<InodeMetadataFunction, Expected<Metadata>(di::This&)> {};

constexpr inline auto inode_metadata = InodeMetadataFunction {};

struct InodeCreateNodeFunction
    : di::Dispatcher<InodeCreateNodeFunction,
                     Expected<di::Arc<TNode>>(di::This&, di::Arc<TNode>, di::TransparentStringView, MetadataType)> {};

constexpr inline auto inode_create_node = InodeCreateNodeFunction {};

struct InodeTruncateFunction : di::Dispatcher<InodeTruncateFunction, Expected<void>(di::This&, u64)> {};

constexpr inline auto inode_truncate = InodeTruncateFunction {};

struct InodeHACKRawDataFunction
    : di::Dispatcher<InodeHACKRawDataFunction, Expected<di::Span<byte const>>(di::This&)> {};

constexpr inline auto inode_hack_raw_data = InodeHACKRawDataFunction {};

using InodeInterface =
    di::meta::List<InodeReadFunction, InodeReadDirectoryFunction, InodeLookupFunction, InodeMetadataFunction,
                   InodeCreateNodeFunction, InodeTruncateFunction, InodeHACKRawDataFunction>;

using InodeImpl = di::Any<InodeInterface>;

class InodeFile {
public:
    explicit InodeFile(di::Arc<TNode> tnode);

    friend di::AnySenderOf<usize> tag_invoke(di::Tag<read_file>, InodeFile& self, UserspaceBuffer<byte> buffer);
    friend Expected<usize> tag_invoke(di::Tag<read_directory>, InodeFile& self, UserspaceBuffer<byte> buffer);
    friend di::AnySenderOf<usize> tag_invoke(di::Tag<write_file>, InodeFile& self, UserspaceBuffer<byte const> buffer);
    friend Expected<Metadata> tag_invoke(di::Tag<file_metadata>, InodeFile& self);
    friend Expected<u64> tag_invoke(di::Tag<seek_file>, InodeFile& self, i64 offset, int whence);
    friend Expected<void> tag_invoke(di::Tag<file_truncate>, InodeFile& self, u64 size);
    friend Expected<di::Span<byte const>> tag_invoke(di::Tag<file_hack_raw_data>, InodeFile& self);

private:
    di::Arc<TNode> m_tnode;
    u64 m_offset { 0 };
};

class Inode : public di::IntrusiveRefCount<Inode> {
public:
    explicit Inode(InodeImpl impl) : m_impl(di::move(impl)) {}

    mm::BackingObject& backing_object() { return m_backing_object; }

    di::Optional<Mount&> mount() const { return m_mount.transform(di::chain(di::dereference, di::ref)); }
    void set_mount(di::Box<Mount> mount) { m_mount = di::move(mount); }

private:
    friend di::AnySenderOf<mm::PhysicalAddress> tag_invoke(di::Tag<inode_read>, Inode& self,
                                                           mm::BackingObject& backing_object, u64 page_number);
    friend Expected<usize> tag_invoke(di::Tag<inode_read_directory>, Inode& self, mm::BackingObject& backing_object,
                                      u64& offset, UserspaceBuffer<byte> buffer);
    friend Expected<di::Arc<TNode>> tag_invoke(di::Tag<inode_lookup>, Inode& self, di::Arc<TNode> parent,
                                               di::TransparentStringView name);
    friend Expected<Metadata> tag_invoke(di::Tag<inode_metadata>, Inode& self);
    friend Expected<di::Arc<TNode>> tag_invoke(di::Tag<inode_create_node>, Inode& self, di::Arc<TNode> parent,
                                               di::TransparentStringView name, MetadataType type);
    friend Expected<void> tag_invoke(di::Tag<inode_truncate>, Inode& self, u64 size);
    friend Expected<di::Span<byte const>> tag_invoke(di::Tag<inode_hack_raw_data>, Inode& self);

    InodeImpl m_impl;
    di::Optional<di::Box<Mount>> m_mount;
    mm::BackingObject m_backing_object;
};
}
