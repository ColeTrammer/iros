#pragma once

#include "di/container/string/prelude.h"
#include "iris/fs/super_block.h"

namespace iris {
class Mount {
public:
    explicit Mount(di::Box<SuperBlock> super_block) : m_super_block(di::move(super_block)) {}

    auto super_block() const -> SuperBlock& { return *m_super_block; }

private:
    di::Box<SuperBlock> m_super_block;
};
}
