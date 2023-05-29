#pragma once

#include <di/any/concepts/any_storage.h>
#include <di/any/container/prelude.h>
#include <di/any/prelude.h>
#include <di/any/storage/hybrid_storage.h>
#include <di/any/storage/storage_category.h>
#include <di/any/vtable/maybe_inline_vtable.h>
#include <di/execution/any/any_env.h>
#include <di/execution/interface/start.h>
#include <di/function/tag_invoke.h>

namespace di::execution {
template<typename Env = void,
         concepts::AnyStorage Storage =
             any::HybridStorage<any::StorageCategory::MoveOnly, 8 * sizeof(void*), alignof(void*)>,
         typename VTablePolicy = any::MaybeInlineVTable<3>>
using AnyOperationState = Any<InterfaceWithEnv<meta::List<types::Tag<execution::start>>, Env>, Storage, VTablePolicy>;
}

namespace di {
using execution::AnyOperationState;
}
