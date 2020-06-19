#pragma once

#include <ext/parse_mode.h>
#include <liim/variant.h>

namespace Ext {

using ModeValue = Variant<Monostate, SymbolicMode, SymbolicMode::Who, Vector<SymbolicMode::Who>, SymbolicMode::Modifier,
                          SymbolicMode::Permission, Vector<SymbolicMode::Permission>, SymbolicMode::Clause::Action,
                          Vector<SymbolicMode::Clause::Action>, SymbolicMode::PermissionCopy, SymbolicMode::Clause>;

}