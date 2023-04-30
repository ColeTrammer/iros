#pragma once

#include <di/container/string/prelude.h>
#include <iris/fs/inode.h>
#include <iris/fs/super_block.h>

namespace iris {
class Mount {
public:
    explicit Mount(di::TransparentString name, di::Box<SuperBlock> super_block)
        : m_name(di::move(name)), m_super_block(di::move(super_block)) {}

    di::TransparentStringView name() const { return m_name.view(); }

    SuperBlock& super_block() const { return *m_super_block; }

private:
    di::TransparentString m_name;
    di::Box<SuperBlock> m_super_block;
};
}
