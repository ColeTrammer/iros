#pragma once

#include <di/container/string/prelude.h>
#include <iris/fs/inode.h>
#include <iris/fs/super_block.h>

namespace iris {
class Mount {
public:
private:
    di::TransparentString m_name;
    di::Box<SuperBlock> m_super_block;
};
}
