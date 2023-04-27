#pragma once

#include <di/any/prelude.h>
#include <di/container/string/prelude.h>
#include <di/vocab/pointer/prelude.h>
#include <iris/fs/inode.h>

namespace iris {
class TNode : public di::IntrusiveRefCount<TNode> {
public:
private:
    di::Arc<TNode> m_parent;
    di::Arc<Inode> m_inode;
    di::TransparentString m_name;
};
}
