#pragma once

#include <di/any/prelude.h>
#include <iris/fs/inode.h>

namespace iris {
class SuperBlock {
public:
private:
    di::Arc<Inode> m_root_inode;
};
}
