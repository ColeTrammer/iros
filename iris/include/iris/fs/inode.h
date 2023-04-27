#pragma once

#include <di/any/dispatch/prelude.h>
#include <di/any/prelude.h>
#include <iris/core/error.h>
#include <iris/fs/file.h>
#include <iris/mm/backing_object.h>

namespace iris {
struct TNode;

struct InodeReadFunction
    : di::Dispatcher<InodeReadFunction, Expected<void>(di::This&, mm::BackingObject&, u64 page_number)> {};

constexpr inline auto inode_read = InodeReadFunction {};

struct InodeLookupFunction
    : di::Dispatcher<InodeLookupFunction,
                     Expected<di::Arc<TNode>>(di::This&, di::Arc<TNode>, di::TransparentStringView)> {};

constexpr inline auto inode_lookup = InodeLookupFunction {};

using InodeInterface = di::meta::List<InodeReadFunction, InodeLookupFunction>;

using InodeImpl = di::Any<InodeInterface>;

class Inode {
public:
private:
    InodeImpl m_impl;
    mm::BackingObject m_backing_object;
};
}
