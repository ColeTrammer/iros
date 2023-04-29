#include <di/container/algorithm/prelude.h>
#include <di/math/prelude.h>
#include <di/vocab/expected/prelude.h>
#include <iris/fs/inode.h>
#include <iris/fs/tnode.h>
#include <iris/mm/map_physical_address.h>
#include <iris/uapi/metadata.h>

namespace iris {
InodeFile::InodeFile(di::Arc<TNode> tnode) : m_tnode(di::move(tnode)) {}

Expected<mm::PhysicalAddress> tag_invoke(di::Tag<inode_read>, Inode& self, mm::BackingObject& backing_object,
                                         u64 page_number) {
    return inode_read(self.m_impl, backing_object, page_number);
}

Expected<di::Arc<TNode>> tag_invoke(di::Tag<inode_lookup>, Inode& self, di::Arc<TNode> parent,
                                    di::TransparentStringView name) {
    return inode_lookup(self.m_impl, di::move(parent), name);
}

Expected<Metadata> tag_invoke(di::Tag<inode_metadata>, Inode& self) {
    return inode_metadata(self.m_impl);
}

Expected<di::Span<byte const>> tag_invoke(di::Tag<inode_hack_raw_data>, Inode& self) {
    return inode_hack_raw_data(self.m_impl);
}

Expected<usize> tag_invoke(di::Tag<read_file>, InodeFile& self, UserspaceBuffer<byte> buffer) {
    auto& inode = *self.m_tnode->inode();
    auto metadata = TRY(inode_metadata(inode));
    auto size = metadata.size;
    if (self.m_offset >= size) {
        return 0;
    }

    auto to_read = di::min(size - self.m_offset, buffer.size());
    auto page_begin = di::align_down(self.m_offset, 4096);
    auto page_end = di::align_up(self.m_offset + to_read, 4096);

    auto& backing_object = inode.backing_object();
    auto nread = 0_u64;
    for (auto offset : di::range(page_begin, page_end) | di::stride(4096)) {
        auto physical_address = backing_object.lock()->lookup_page(offset / 4096);
        if (!physical_address) {
            physical_address = TRY(inode_read(inode, backing_object, offset / 4096));
        }
        ASSERT(physical_address);

        auto page_offset = offset % 4096;
        auto to_read = di::min(4096 - page_offset, buffer.size() - nread);

        auto page = TRY(mm::map_physical_address(*physical_address, 4096));
        auto page_data = di::Span { &page.typed<byte const>() + page_offset, to_read };

        TRY(buffer.write(*page_data.subspan(page_offset, to_read)));
        buffer.advance(to_read);

        nread += to_read;
        self.m_offset += to_read;
    }
    return nread;
}

Expected<usize> tag_invoke(di::Tag<read_directory>, InodeFile&, UserspaceBuffer<byte>) {
    return di::Unexpected(Error::OperationNotSupported);
}

Expected<usize> tag_invoke(di::Tag<write_file>, InodeFile&, UserspaceBuffer<byte const>) {
    return di::Unexpected(Error::OperationNotSupported);
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

Expected<di::Span<byte const>> tag_invoke(di::Tag<file_hack_raw_data>, InodeFile& self) {
    auto& inode = *self.m_tnode->inode();
    return inode_hack_raw_data(inode);
}
}
