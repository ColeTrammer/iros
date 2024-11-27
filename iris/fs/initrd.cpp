#include <iris/fs/initrd.h>

#include <di/any/concepts/prelude.h>
#include <di/container/tree/prelude.h>
#include <di/execution/algorithm/just.h>
#include <di/math/prelude.h>
#include <di/vocab/expected/prelude.h>
#include <iris/core/global_state.h>
#include <iris/core/print.h>
#include <iris/fs/inode.h>
#include <iris/mm/backing_object.h>
#include <iris/uapi/directory.h>
#include <iris/uapi/initrd.h>
#include <iris/uapi/metadata.h>

namespace iris {
class DirentIterator
    : public di::container::IteratorBase<DirentIterator, di::ForwardIteratorTag, initrd::DirectoryEntry, isize> {
public:
    DirentIterator() = default;

    explicit DirentIterator(byte const* data, bool at_end = false) : m_data(data), m_at_end(at_end) {}

    auto operator*() const -> initrd::DirectoryEntry const& {
        return *reinterpret_cast<initrd::DirectoryEntry const*>(m_data);
    }

    void advance_one() {
        if ((**this).next_entry == 0) {
            m_at_end = true;
        } else {
            // FIXME: ensure this is actually valid.
            m_data += (**this).next_entry;
        }
    }

    auto operator==(di::DefaultSentinel) const -> bool { return m_at_end; }

    auto data() const -> byte const* { return m_data; }

private:
    byte const* m_data;
    bool m_at_end { false };
};

struct InitrdInodeImpl {
    // NOTE: this data is directly mapped from physical memory.
    di::Span<byte const> data;

    initrd::Type type { initrd::Type::Regular };

    // FIXME: this really should use a kernel-level inode cache.
    di::TreeMap<di::TransparentString, di::Arc<Inode>> inodes;

    friend auto tag_invoke(di::Tag<inode_read>, InitrdInodeImpl& self, mm::BackingObject& object, u64 page_number)
        -> di::AnySenderOf<mm::PhysicalAddress> {
        auto virtual_address = di::to_uintptr(self.data.data() + page_number * 4096);
        virtual_address -= global_state().virtual_to_physical_offset.raw_value();
        auto physical_address = mm::PhysicalAddress(virtual_address);

        object.lock()->add_page(physical_address, page_number);
        return physical_address;
    }

    friend auto tag_invoke(di::Tag<inode_read_directory>, InitrdInodeImpl& self, mm::BackingObject&, u64& offset,
                           UserspaceBuffer<byte> buffer) -> di::AnySenderOf<usize> {
        auto it = DirentIterator(self.data.data() + offset, offset == self.data.size());
        if (it == di::default_sentinel) {
            return 0;
        }

        auto const& entry = *it;

        auto storage = di::Array<byte, sizeof(DirectoryRecord) + 256> {};
        auto effective_size = sizeof(DirectoryRecord) + di::align_up(entry.name_length, 8);
        auto* dirent = reinterpret_cast<DirectoryRecord*>(storage.data());
        dirent->inode = 0;
        dirent->offset = offset;
        dirent->type = MetadataType(di::to_underlying(entry.type));
        dirent->name_length = entry.name_length;
        dirent->size = effective_size;

        auto* name_buffer = const_cast<char*>(dirent->name().data());
        di::copy(entry.name(), name_buffer);

        TRY(buffer.write(di::Span { storage.data(), effective_size }));

        it++;
        if (it == di::default_sentinel) {
            offset = self.data.size();
        } else {
            offset = it.data() - self.data.data();
        }

        return effective_size;
    }

    friend auto tag_invoke(di::Tag<inode_lookup>, InitrdInodeImpl& self, di::Arc<TNode> parent,
                           di::TransparentStringView name) -> di::AnySenderOf<di::Arc<TNode>> {
        auto result = self.inodes.find(name);
        if (result == self.inodes.end()) {
            return di::Unexpected(Error::NoSuchFileOrDirectory);
        }

        return di::make_arc<TNode>(di::move(parent), di::get<1>(*result), TRY(name | di::to<di::TransparentString>()));
    }

    friend auto tag_invoke(di::Tag<inode_metadata>, InitrdInodeImpl& self) -> di::AnySenderOf<Metadata> {
        return Metadata { .type = MetadataType(di::to_underlying(self.type)), .size = self.data.size() };
    }

    friend auto tag_invoke(di::Tag<inode_create_node>, InitrdInodeImpl&, di::Arc<TNode> const&,
                           di::TransparentStringView, MetadataType) -> di::AnySenderOf<di::Arc<TNode>> {
        return di::Unexpected(Error::ReadOnlyFileSystem);
    }

    friend auto tag_invoke(di::Tag<inode_truncate>, InitrdInodeImpl&, u64) -> di::AnySenderOf<void> {
        return di::Unexpected(Error::ReadOnlyFileSystem);
    }

    friend auto tag_invoke(di::Tag<inode_hack_raw_data>, InitrdInodeImpl& self)
        -> di::AnySenderOf<di::Span<byte const>> {
        return self.data;
    }
};

static_assert(di::Impl<InitrdInodeImpl, InodeInterface>);

auto init_initrd() -> Expected<void> {
    auto& global_state = global_state_in_boot();
    auto initrd = global_state.initrd;
    auto const& super_block = *initrd.typed_pointer_unchecked<initrd::SuperBlock>(0);
    if (super_block.signature != initrd::signature) {
        println("Initrd has invalid signature: {}"_sv, super_block.signature);
        return di::Unexpected(Error::InvalidArgument);
    }

    auto data_from_dirent = [&](initrd::DirectoryEntry const& entry) {
        return *initrd.subspan(initrd::block_size * entry.block_offset, entry.byte_size);
    };

    auto const& root_dirent = super_block.root_directory;
    auto root_data = data_from_dirent(root_dirent);

    auto visit = [&](auto&& visit, initrd::DirectoryEntry const& dirent,
                     di::Span<byte const> data) -> Expected<di::Arc<Inode>> {
        if (dirent.type != initrd::Type::Directory) {
            return di::make_arc<Inode>(TRY(InodeImpl::create(InitrdInodeImpl { data, dirent.type, {} })));
        }

        auto inode_impl = InitrdInodeImpl { data, dirent.type, {} };
        for (auto it = DirentIterator(data.data(), data.empty()); it != di::default_sentinel; ++it) {
            auto const& entry = *it;
            auto name = TRY(entry.name() | di::to<di::TransparentString>());
            auto child_data = data_from_dirent(entry);

            println("Adding initrd child /{}"_sv, name);
            auto child_inode = TRY(visit(visit, entry, child_data));
            TRY(inode_impl.inodes.try_emplace(di::move(name), di::move(child_inode)));
        }
        return di::make_arc<Inode>(TRY(InodeImpl::create(di::move(inode_impl))));
    };

    println("Construct initrd root..."_sv);
    auto root_inode = TRY(visit(visit, root_dirent, root_data));
    global_state.initrd_root = TRY(di::make_arc<TNode>(nullptr, di::move(root_inode), di::TransparentString {}));
    return {};
}
}
