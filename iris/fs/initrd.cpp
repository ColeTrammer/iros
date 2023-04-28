#include <di/any/concepts/prelude.h>
#include <di/container/tree/prelude.h>
#include <di/math/prelude.h>
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

    // FIXME: this really should use a kernel-level inode cache.
    di::TreeMap<di::TransparentString, di::Arc<Inode>> inodes;

    friend Expected<void> tag_invoke(di::Tag<inode_read>, InitrdInodeImpl& self, mm::BackingObject& object,
                                     u64 page_number) {
        auto virtual_address = di::to_uintptr(self.data.data() + page_number * 4096);
        virtual_address -= global_state().virtual_to_physical_offset.raw_value();
        auto physical_address = mm::PhysicalAddress(virtual_address);

        object.lock()->add_page(physical_address, page_number);
        return {};
    }

    friend Expected<di::Arc<TNode>> tag_invoke(di::Tag<inode_lookup>, InitrdInodeImpl& self, di::Arc<TNode> parent,
                                               di::TransparentStringView name) {
        auto result = self.inodes.find(name);
        if (result == self.inodes.end()) {
            return di::Unexpected(Error::NoSuchFileOrDirectory);
        }

        return di::try_make_arc<TNode>(di::move(parent), di::get<1>(*result),
                                       TRY(name | di::to<di::TransparentString>()));
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
            return di::try_make_arc<Inode>(InodeImpl::create(InitrdInodeImpl { data, {} }));
        }

        auto inode_impl = InitrdInodeImpl { data, {} };
        for (auto it = DirentIterator(data.data()); it != di::default_sentinel; ++it) {
            auto const& entry = *it;
            auto name = TRY(entry.name() | di::to<di::TransparentString>());
            auto child_data = data_from_dirent(entry);

            println("Adding initrd child /{}"_sv, name);
            auto child_inode = TRY(visit(visit, entry, child_data));
            TRY(inode_impl.inodes.try_emplace(di::move(name), di::move(child_inode)));
        }
        return di::try_make_arc<Inode>(InodeImpl::create(di::move(inode_impl)));
    };

    println("Construct initrd root..."_sv);
    auto root_inode = TRY(visit(visit, root_dirent, root_data));
    global_state.initrd_root = TRY(di::try_make_arc<TNode>(nullptr, di::move(root_inode), di::TransparentString {}));
    return {};
}

Expected<di::Tuple<di::Span<byte const>, initrd::Type>> lookup_in_initrd(di::PathView path) {
    if (!path.is_absolute()) {
        return di::Unexpected(Error::OperationNotSupported);
    }

    auto initrd = global_state().initrd;
    auto& super_block = *initrd.typed_pointer_unchecked<initrd::SuperBlock>(0);
    if (super_block.signature != initrd::signature) {
        println("Initrd has invalid signature: {}"_sv, super_block.signature);
        return di::Unexpected(Error::InvalidArgument);
    }

    auto data_from_dirent = [&](initrd::DirectoryEntry const& entry) {
        return *initrd.subspan(initrd::block_size * entry.block_offset, entry.byte_size);
    };

    auto* current = &super_block.root_directory;
    path = *path.strip_prefix("/"_pv);
    while (!path.empty()) {
        auto first = *path.begin();

        if (current->type != initrd::Type::Directory) {
            println("Failed lookup because not a directory."_sv);
            return di::Unexpected(Error::NotADirectory);
        }

        auto data = data_from_dirent(*current);
        auto it = DirentIterator(data.data());
        auto sent = di::default_sentinel;

        auto result = di::find(it, sent, first, &initrd::DirectoryEntry::name);
        if (result == sent) {
            println("Failed to find {} in directory."_sv, first);
            return di::Unexpected(Error::NoSuchFileOrDirectory);
        }

        current = &*result;
        path = *path.strip_prefix(first);
    }

    return di::make_tuple(data_from_dirent(*current), current->type);
}

class InitrdFile {
public:
    constexpr explicit InitrdFile(di::Span<byte const> data) : m_data(data) {}

private:
    friend Expected<usize> tag_invoke(di::Tag<read_file>, InitrdFile& file, UserspaceBuffer<byte> buffer) {
        auto to_read = di::min(buffer.size(), file.m_data.size() - file.m_offset);
        TRY(buffer.write(*file.m_data.subspan(file.m_offset, to_read)));
        file.m_offset += to_read;
        return to_read;
    }

    friend Expected<usize> tag_invoke(di::Tag<read_directory>, InitrdFile& file, UserspaceBuffer<byte> buffer) {
        auto it = DirentIterator(file.m_data.data() + file.m_offset, file.m_offset == file.m_data.size());
        if (it == di::default_sentinel) {
            return 0;
        }

        auto const& entry = *it;

        auto storage = di::Array<byte, sizeof(DirectoryRecord) + 256> {};
        auto effective_size = sizeof(DirectoryRecord) + di::align_up(entry.name_length, 8);
        auto* dirent = reinterpret_cast<DirectoryRecord*>(storage.data());
        dirent->inode = 0;
        dirent->offset = file.m_offset;
        dirent->type = MetadataType(di::to_underlying(entry.type));
        dirent->name_length = entry.name_length;
        dirent->size = effective_size;

        auto* name_buffer = const_cast<char*>(dirent->name().data());
        di::copy(entry.name(), name_buffer);

        TRY(buffer.write(di::Span { storage.data(), effective_size }));

        it++;
        if (it == di::default_sentinel) {
            file.m_offset = file.m_data.size();
        } else {
            file.m_offset = it.data() - file.m_data.data();
        }

        return effective_size;
    }

    friend Expected<i64> tag_invoke(di::Tag<seek_file>, InitrdFile& file, i64 offset, int whence) {
        switch (whence) {
            case 0:
                file.m_offset = offset;
                return file.m_offset;
            case 1:
                file.m_offset += offset;
                return file.m_offset;
            case 2:
                file.m_offset = file.m_data.size() + offset;
                return file.m_offset;
        }
        return di::Unexpected(Error::InvalidArgument);
    }

    di::Span<byte const> m_data;
    u64 m_offset { 0 };
};

Expected<File> open_in_initrd(di::PathView path) {
    auto [data, type] = TRY(lookup_in_initrd(path));
    return File::try_create(InitrdFile { data });
}

Expected<Metadata> path_metadata_in_initrd(di::PathView path) {
    auto [data, type] = TRY(lookup_in_initrd(path));
    return Metadata { .type = MetadataType(di::to_underlying(type)), .size = data.size() };
}
}
