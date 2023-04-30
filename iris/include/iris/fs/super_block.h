#pragma once

#include <di/vocab/pointer/prelude.h>

namespace iris {
class Inode;

class SuperBlock {
public:
    explicit SuperBlock(di::Arc<Inode> root_inode);
    ~SuperBlock();

    di::Arc<Inode> root_inode() const;

private:
    di::Arc<Inode> m_root_inode;
};
}
