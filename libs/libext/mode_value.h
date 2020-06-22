#pragma once

#include <ext/parse_mode.h>
#include <liim/variant.h>

namespace Ext {

using ModeValue =
    LIIM::Variant<ModeLiteral, SymbolicMode, Clause, Wholist, Who, Actionlist, Action, PermissionCopy, Modifier, Permlist, Permission>;

}
