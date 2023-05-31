#include <di/container/algorithm/prelude.h>
#include <di/execution/algorithm/sync_wait.h>
#include <di/execution/any/any_sender.h>
#include <di/execution/macro/try_or_send_error.h>
#include <di/math/prelude.h>
#include <di/vocab/expected/prelude.h>
#include <iris/fs/inode.h>
#include <iris/fs/tnode.h>
#include <iris/mm/map_physical_address.h>
#include <iris/mm/physical_address.h>
#include <iris/uapi/metadata.h>

namespace iris {
InodeFile::InodeFile(di::Arc<TNode> tnode) : m_tnode(di::move(tnode)) {}

di::AnySenderOf<mm::PhysicalAddress> tag_invoke(di::Tag<inode_read>, Inode& self, mm::BackingObject& backing_object,
                                                u64 page_number) {
    return inode_read(self.m_impl, backing_object, page_number);
}

Expected<usize> tag_invoke(di::Tag<inode_read_directory>, Inode& self, mm::BackingObject& backing_object, u64& offset,
                           UserspaceBuffer<byte> buffer) {
    return inode_read_directory(self.m_impl, backing_object, offset, buffer);
}

Expected<di::Arc<TNode>> tag_invoke(di::Tag<inode_lookup>, Inode& self, di::Arc<TNode> parent,
                                    di::TransparentStringView name) {
    return inode_lookup(self.m_impl, di::move(parent), name);
}

Expected<Metadata> tag_invoke(di::Tag<inode_metadata>, Inode& self) {
    return inode_metadata(self.m_impl);
}

Expected<di::Arc<TNode>> tag_invoke(di::Tag<inode_create_node>, Inode& self, di::Arc<TNode> parent,
                                    di::TransparentStringView name, MetadataType type) {
    return inode_create_node(self.m_impl, di::move(parent), name, type);
}

Expected<void> tag_invoke(di::Tag<inode_truncate>, Inode& self, u64 size) {
    return inode_truncate(self.m_impl, size);
}

Expected<di::Span<byte const>> tag_invoke(di::Tag<inode_hack_raw_data>, Inode& self) {
    return inode_hack_raw_data(self.m_impl);
}

di::AnySenderOf<usize> tag_invoke(di::Tag<read_file>, InodeFile& self, UserspaceBuffer<byte> buffer) {
    auto& inode = *self.m_tnode->inode();
    auto metadata = TRY_OR_SEND_ERROR(inode_metadata(inode));
    auto size = metadata.size;
    if (self.m_offset >= size) {
        return di::execution::just(0);
    }

    auto to_read = di::min(size - self.m_offset, buffer.size());
    auto page_begin = di::align_down(self.m_offset, 4096);
    auto page_end = di::align_up(self.m_offset + to_read, 4096);

    auto& backing_object = inode.backing_object();
    auto nread = 0_u64;
    for (auto offset : di::range(page_begin, page_end) | di::stride(4096)) {
        auto physical_address = backing_object.lock()->lookup_page(offset / 4096);
        if (!physical_address) {
            physical_address =
                TRY_OR_SEND_ERROR(di::execution::sync_wait(inode_read(inode, backing_object, offset / 4096)));
        }
        ASSERT(physical_address);

        auto page_offset = offset % 4096;
        auto to_read = di::min(4096 - page_offset, buffer.size() - nread);

        auto page = TRY_OR_SEND_ERROR(mm::map_physical_address(*physical_address, 4096));
        auto page_data = di::Span { &page.typed<byte const>() + page_offset, to_read };

        TRY_OR_SEND_ERROR(buffer.write(*page_data.subspan(page_offset, to_read)));
        buffer.advance(to_read);

        nread += to_read;
        self.m_offset += to_read;
    }
    return di::execution::just(nread);
}

Expected<usize> tag_invoke(di::Tag<read_directory>, InodeFile& self, UserspaceBuffer<byte> buffer) {
    auto& inode = *self.m_tnode->inode();
    return inode_read_directory(inode, inode.backing_object(), self.m_offset, buffer);
}

Expected<usize> tag_invoke(di::Tag<write_file>, InodeFile& self, UserspaceBuffer<byte const> buffer) {
    auto& inode = *self.m_tnode->inode();
    auto metadata = TRY(inode_metadata(inode));
    auto size = metadata.size;

    auto page_begin = di::align_down(self.m_offset, 4096);
    auto page_end = di::align_up(self.m_offset + buffer.size(), 4096);

    auto& backing_object = inode.backing_object();
    auto nwritten = 0_u64;
    for (auto offset : di::range(page_begin, page_end) | di::stride(4096)) {
        // FIXME: this implementation is suboptimal in the case where we are writing an entire page, since it
        // unconditionaly reads the page from disk. This can be skipped, but requires a more complex VFS interface.
        // Also, the code assumes that we can read past the end of the inode. Although we make sure to update the size
        // at the end of this function, this is probably sketchy for real file systems.
        auto physical_address = backing_object.lock()->lookup_page(offset / 4096);
        if (!physical_address) {
            physical_address =
                TRY_UNERASE_ERROR(di::execution::sync_wait(inode_read(inode, backing_object, offset / 4096)));
        }
        ASSERT(physical_address);

        auto page_offset = offset % 4096;
        auto to_write = di::min(4096 - page_offset, buffer.size() - nwritten);

        auto page = TRY(mm::map_physical_address(*physical_address, 4096));
        auto page_data = di::Span { &page.typed<byte>() + page_offset, to_write };

        TRY(buffer.copy_to(*page_data.subspan(page_offset, to_write)));
        buffer.advance(to_write);

        nwritten += to_write;
        self.m_offset += to_write;
    }

    if (self.m_offset > size) {
        TRY(inode_truncate(inode, self.m_offset));
    }
    return nwritten;
}

Expected<Metadata> tag_invoke(di::Tag<file_metadata>, InodeFile& self) {
    auto& inode = *self.m_tnode->inode();
    return inode_metadata(inode);
}

Expected<u64> tag_invoke(di::Tag<seek_file>, InodeFile& self, i64 offset, int whence) {
    switch (whence) {
        case 0:
            self.m_offset = offset;
            return self.m_offset;
        case 1:
            self.m_offset += offset;
            return self.m_offset;
        case 2: {
            auto& inode = *self.m_tnode->inode();
            auto metadata = TRY(inode_metadata(inode));
            self.m_offset = metadata.size + offset;
            return self.m_offset;
        }
    }
    return di::Unexpected(Error::InvalidArgument);
}

Expected<void> tag_invoke(di::Tag<file_truncate>, InodeFile& self, u64 size) {
    auto& inode = *self.m_tnode->inode();
    return inode_truncate(inode, size);
}

Expected<di::Span<byte const>> tag_invoke(di::Tag<file_hack_raw_data>, InodeFile& self) {
    auto& inode = *self.m_tnode->inode();
    return inode_hack_raw_data(inode);
}
}
