#include <iris/core/global_state.h>
#include <iris/core/print.h>
#include <iris/fs/initrd.h>
#include <iris/uapi/initrd.h>

namespace iris {
Expected<di::Span<di::Byte const>> lookup_in_initrd(di::PathView path) {
    if (!path.is_absolute()) {
        return di::Unexpected(Error::OutOfMemory);
    }

    println("Looking up: {}"_sv, path);

    auto initrd = global_state().initrd;
    auto& super_block = *initrd.typed_pointer_unchecked<initrd::SuperBlock>(0);
    if (super_block.signature != initrd::signature) {
        println("Initrd has invalid signature: {}"_sv, super_block.signature);
        return di::Unexpected(Error::OutOfMemory);
    }

    class DirentIterator
        : public di::container::IteratorBase<DirentIterator, di::ForwardIteratorTag, initrd::DirectoryEntry, isize> {
    public:
        DirentIterator() = default;

        explicit DirentIterator(di::Byte const* data) : m_data(data) {}

        initrd::DirectoryEntry const& operator*() const {
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

        bool operator==(di::DefaultSentinel) const { return m_at_end; }

    private:
        di::Byte const* m_data;
        bool m_at_end { false };
    };

    auto data_from_dirent = [&](initrd::DirectoryEntry const& entry) {
        return *initrd.subspan(initrd::block_size * entry.block_offset, entry.byte_size);
    };

    auto* current = &super_block.root_directory;
    path = *path.strip_prefix("/"_pv);
    while (!path.empty()) {
        auto first = *path.begin();

        if (current->type != initrd::Type::Directory) {
            println("Failed lookup because not a directory."_sv);
            return di::Unexpected(Error::OutOfMemory);
        }

        auto data = data_from_dirent(*current);
        auto it = DirentIterator(data.data());
        auto sent = di::default_sentinel;

        di::for_each(it, sent, [&](auto& entry) {
            println("Found {}"_sv, entry.name());
        });

        auto result = di::find(it, sent, first, &initrd::DirectoryEntry::name);
        if (result == sent) {
            println("Failed to find {} in directory."_sv, first);
            return di::Unexpected(Error::OutOfMemory);
        }

        current = &*result;
        path = *path.strip_prefix(first);
    }

    if (current->type != initrd::Type::Regular) {
        println("Failed lookup because not a regular file."_sv);
        return di::Unexpected(Error::OutOfMemory);
    }
    return data_from_dirent(*current);
}
}