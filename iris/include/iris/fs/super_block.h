#pragma once

#include "di/vocab/pointer/prelude.h"

namespace iris {
class Inode;

class SuperBlock {
public:
    explicit SuperBlock(di::Arc<Inode> root_inode);
    ~SuperBlock();

    auto root_inode() const -> di::Arc<Inode>;

private:
    di::Arc<Inode> m_root_inode;
};
}
