#pragma once

#include "di/any/concepts/interface.h"
#include "di/any/meta/merge_interfaces.h"
#include "di/any/types/method.h"
#include "di/any/types/this.h"
#include "di/execution/interface/get_env.h"
#include "di/function/tag_invoke.h"
#include "di/meta/core.h"
#include "di/meta/language.h"

namespace di::execution {
template<concepts::Interface Interface, typename Env>
using InterfaceWithEnv = meta::Conditional<
    concepts::LanguageVoid<Env>, Interface,
    meta::MergeInterfaces<Interface,
                          meta::List<types::Method<types::Tag<execution::get_env>, Env(types::This const&)>>>>;
}
