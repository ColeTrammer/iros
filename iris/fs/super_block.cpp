#include <iris/fs/super_block.h>

#include <iris/fs/inode.h>

namespace iris {
SuperBlock::SuperBlock(di::Arc<Inode> root_inode) : m_root_inode(di::move(root_inode)) {}

SuperBlock::~SuperBlock() = default;

auto SuperBlock::root_inode() const -> di::Arc<Inode> {
    return m_root_inode;
}
}
