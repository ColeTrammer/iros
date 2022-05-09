#pragma once

#include <liim/option.h>
#include <liim/string.h>
#include <liim/variant.h>
#include <liim/vector.h>
#include <sys/types.h>

namespace Ext {

using ModeTerminal = Monostate;

enum class Who {
    User = 2,
    Group = 1,
    Other = 0,
    All = 3,
};

enum class PermissionCopy {
    User = 2,
    Group = 1,
    Other = 0,
};

enum class Permission {
    Read = 4,
    Write = 2,
    Execute = 1,
    Search = 5,
    SetID,
    Sticky,
};

enum class Modifier {
    Plus,
    Minus,
    Equal,
};

using Permlist = Vector<Permission>;

struct Action {
    Modifier modifier;
    Variant<Monostate, PermissionCopy, Permlist> copy_or_permission_list;

    mode_t resolve(mode_t reference, mode_t mask, const Vector<Who>& who_list) const;
};

using Wholist = Vector<Who>;
using Actionlist = Vector<Action>;

class Clause {
public:
    Clause(Wholist&& who_list, Actionlist&& action_list) : m_who_list(move(who_list)), m_action_list(move(action_list)) {}

    Wholist& who_list() { return m_who_list; }
    const Wholist& who_list() const { return m_who_list; }

    Actionlist& action_list() { return m_action_list; }
    const Actionlist& action_list() const { return m_action_list; }

private:
    Wholist m_who_list;
    Actionlist m_action_list;
};

class SymbolicMode {
public:
    Vector<Clause>& clauses() { return m_clauses; }
    const Vector<Clause>& clauses() const { return m_clauses; }

    mode_t resolve(mode_t reference, mode_t mask) const;

private:
    Vector<Clause> m_clauses;
};

class Mode {
public:
    Mode(mode_t mode = 0) : m_mode(mode) {}
    Mode(const SymbolicMode& mode) : m_mode(SymbolicMode(mode)) {}

    Variant<mode_t, SymbolicMode>& impl() { return m_mode; }
    const Variant<mode_t, SymbolicMode>& impl() const { return m_mode; }

    mode_t resolve(mode_t reference, Option<mode_t> umask_value = {}) const;

private:
    Variant<mode_t, SymbolicMode> m_mode;
};

Option<Mode> parse_mode(const String& string);

}
