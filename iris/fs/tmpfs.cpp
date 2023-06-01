#include <di/container/string/prelude.h>
#include <di/container/tree/prelude.h>
#include <di/execution/algorithm/just_or_error.h>
#include <di/execution/algorithm/sync_wait.h>
#include <di/execution/macro/try_or_send_error.h>
#include <di/math/prelude.h>
#include <di/vocab/expected/prelude.h>
#include <di/vocab/pointer/prelude.h>
#include <iris/core/error.h>
#include <iris/core/global_state.h>
#include <iris/fs/inode.h>
#include <iris/fs/path.h>
#include <iris/fs/tmpfs.h>
#include <iris/fs/tnode.h>
#include <iris/mm/backing_object.h>
#include <iris/mm/page_frame_allocator.h>
#include <iris/mm/physical_address.h>
#include <iris/uapi/directory.h>
#include <iris/uapi/metadata.h>

namespace iris {
struct TmpfsInodeImpl {
    Metadata metadata;
    di::Vector<di::Tuple<di::TransparentString, di::Arc<Inode>>> inodes;

    friend di::AnySenderOf<mm::PhysicalAddress> tag_invoke(di::Tag<inode_read>, TmpfsInodeImpl&,
                                                           mm::BackingObject& backing_object, u64 page_number) {
        // NOTE: if we're getting here, it means that the page is not present in the backing object. Since this is the
        // tmpfs, just allocate a new (zero-filled) page and add it to the backing object.
        auto page = TRY_OR_SEND_ERROR(mm::allocate_page_frame());
        backing_object.lock()->add_page(page, page_number);
        return di::execution::just(page);
    }

    friend di::AnySenderOf<usize> tag_invoke(di::Tag<inode_read_directory>, TmpfsInodeImpl& self, mm::BackingObject&,
                                             u64& offset, UserspaceBuffer<byte> buffer) {
        auto const* it = self.inodes.iterator(offset);
        if (it == self.inodes.end()) {
            co_return 0;
        }

        auto const& entry = *it;

        auto child_metadata = co_await inode_metadata(*di::get<1>(entry));
        auto const& name = di::get<0>(entry);
        auto name_length = name.size();

        auto storage = di::Array<byte, sizeof(DirectoryRecord) + 256> {};
        auto effective_size = sizeof(DirectoryRecord) + di::align_up(name_length, 8);
        auto* dirent = reinterpret_cast<DirectoryRecord*>(storage.data());
        dirent->inode = 0;
        dirent->offset = offset;
        dirent->type = MetadataType(child_metadata.type);
        dirent->name_length = name_length;
        dirent->size = effective_size;

        auto* name_buffer = const_cast<char*>(dirent->name().data());
        di::copy(name, name_buffer);

        co_await buffer.write(di::Span { storage.data(), effective_size });

        it++;
        if (it == self.inodes.end()) {
            offset = self.inodes.size();
        } else {
            offset = it - self.inodes.begin();
        }

        co_return effective_size;
    }

    friend di::AnySenderOf<di::Arc<TNode>> tag_invoke(di::Tag<inode_lookup>, TmpfsInodeImpl& self,
                                                      di::Arc<TNode> parent, di::TransparentStringView name) {
        auto const* it = di::find_if(self.inodes, [&](auto const& entry) {
            return di::get<0>(entry) == name;
        });
        if (it == self.inodes.end()) {
            return di::execution::just_error(Error::NoSuchFileOrDirectory);
        }
        return di::execution::just_or_error(
            di::make_arc<TNode>(di::move(parent), di::get<1>(*it), TRY_OR_SEND_ERROR(name.to_owned())));
    }

    friend di::AnySenderOf<Metadata> tag_invoke(di::Tag<inode_metadata>, TmpfsInodeImpl& self) {
        return di::execution::just(self.metadata);
    }

    friend di::AnySenderOf<di::Arc<TNode>> tag_invoke(di::Tag<inode_create_node>, TmpfsInodeImpl& self,
                                                      di::Arc<TNode> const& parent, di::TransparentStringView name,
                                                      MetadataType type) {
        auto& child = TRY_OR_SEND_ERROR(self.inodes.emplace_back(
            TRY_OR_SEND_ERROR(name.to_owned()),
            TRY_OR_SEND_ERROR(di::make_arc<Inode>(
                TRY_OR_SEND_ERROR(InodeImpl::create(TmpfsInodeImpl(Metadata { .type = type, .size = 0 }, {})))))));
        return di::execution::just_or_error(
            di::make_arc<TNode>(parent, di::get<1>(child), TRY_OR_SEND_ERROR(name.to_owned())));
    }

    friend di::AnySenderOf<> tag_invoke(di::Tag<inode_truncate>, TmpfsInodeImpl& self, u64 size) {
        if (self.metadata.type != MetadataType::Regular) {
            return di::execution::just_error(Error::OperationNotSupported);
        }
        self.metadata.size = size;
        return di::execution::just();
    }

    friend di::AnySenderOf<di::Span<byte const>> tag_invoke(di::Tag<inode_hack_raw_data>, TmpfsInodeImpl&) {
        return di::execution::just_error(Error::OperationNotSupported);
    }
};

static_assert(di::Impl<TmpfsInodeImpl, InodeInterface>);

Expected<void> init_tmpfs() {
    auto& global_state = global_state_in_boot();
    auto& initrd_root = global_state.initrd_root;

    auto root_inode = TRY(di::make_arc<Inode>(
        TRY(InodeImpl::create(TmpfsInodeImpl(Metadata { .type = MetadataType::Directory, .size = 0 }, {})))));
    auto super_block = TRY(di::make_box<SuperBlock>(root_inode));
    auto mount = TRY(di::make_box<Mount>(di::move(super_block)));

    auto tmp_tnode = TRY_UNERASE_ERROR(di::execution::sync_wait(lookup_path(initrd_root, initrd_root, "/tmp"_pv)));
    tmp_tnode->inode()->set_mount(di::move(mount));
    return {};
}
}
