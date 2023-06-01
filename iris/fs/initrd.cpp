#include <di/any/concepts/prelude.h>
#include <di/container/tree/prelude.h>
#include <di/execution/algorithm/just.h>
#include <di/execution/algorithm/just_or_error.h>
#include <di/execution/macro/try_or_send_error.h>
#include <di/math/prelude.h>
#include <di/vocab/expected/prelude.h>
#include <iris/core/global_state.h>
#include <iris/core/print.h>
#include <iris/fs/initrd.h>
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

    initrd::DirectoryEntry const& operator*() const { return *reinterpret_cast<initrd::DirectoryEntry const*>(m_data); }

    void advance_one() {
        if ((**this).next_entry == 0) {
            m_at_end = true;
        } else {
            // FIXME: ensure this is actually valid.
            m_data += (**this).next_entry;
        }
    }

    bool operator==(di::DefaultSentinel) const { return m_at_end; }

    byte const* data() const { return m_data; }

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

    friend di::AnySenderOf<mm::PhysicalAddress> tag_invoke(di::Tag<inode_read>, InitrdInodeImpl& self,
                                                           mm::BackingObject& object, u64 page_number) {
        auto virtual_address = di::to_uintptr(self.data.data() + page_number * 4096);
        virtual_address -= global_state().virtual_to_physical_offset.raw_value();
        auto physical_address = mm::PhysicalAddress(virtual_address);

        object.lock()->add_page(physical_address, page_number);
        return di::execution::just(physical_address);
    }

    friend di::AnySenderOf<usize> tag_invoke(di::Tag<inode_read_directory>, InitrdInodeImpl& self, mm::BackingObject&,
                                             u64& offset, UserspaceBuffer<byte> buffer) {
        auto it = DirentIterator(self.data.data() + offset, offset == self.data.size());
        if (it == di::default_sentinel) {
            return di::execution::just(0);
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

        TRY_OR_SEND_ERROR(buffer.write(di::Span { storage.data(), effective_size }));

        it++;
        if (it == di::default_sentinel) {
            offset = self.data.size();
        } else {
            offset = it.data() - self.data.data();
        }

        return di::execution::just(effective_size);
    }

    friend di::AnySenderOf<di::Arc<TNode>> tag_invoke(di::Tag<inode_lookup>, InitrdInodeImpl& self,
                                                      di::Arc<TNode> parent, di::TransparentStringView name) {
        auto result = self.inodes.find(name);
        if (result == self.inodes.end()) {
            return di::execution::just_error(Error::NoSuchFileOrDirectory);
        }

        return di::execution::just_or_error(di::make_arc<TNode>(
            di::move(parent), di::get<1>(*result), TRY_OR_SEND_ERROR(name | di::to<di::TransparentString>())));
    }

    friend di::AnySenderOf<Metadata> tag_invoke(di::Tag<inode_metadata>, InitrdInodeImpl& self) {
        return di::execution::just(
            Metadata { .type = MetadataType(di::to_underlying(self.type)), .size = self.data.size() });
    }

    friend di::AnySenderOf<di::Arc<TNode>> tag_invoke(di::Tag<inode_create_node>, InitrdInodeImpl&,
                                                      di::Arc<TNode> const&, di::TransparentStringView, MetadataType) {
        return di::execution::just_error(Error::ReadOnlyFileSystem);
    }

    friend di::AnySenderOf<void> tag_invoke(di::Tag<inode_truncate>, InitrdInodeImpl&, u64) {
        return di::execution::just_error(Error::ReadOnlyFileSystem);
    }

    friend di::AnySenderOf<di::Span<byte const>> tag_invoke(di::Tag<inode_hack_raw_data>, InitrdInodeImpl& self) {
        return di::execution::just(self.data);
    }
};

static_assert(di::Impl<InitrdInodeImpl, InodeInterface>);

Expected<void> init_initrd() {
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
