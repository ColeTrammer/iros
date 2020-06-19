#pragma once

#include <liim/maybe.h>
#include <liim/string.h>
#include <liim/variant.h>
#include <liim/vector.h>
#include <sys/types.h>

namespace Ext {

class SymbolicMode {
public:
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

    class Clause {
    public:
        struct Action {
            Modifier modifier;
            Variant<Monostate, PermissionCopy, Vector<Permission>> copy_or_permission_list;

            mode_t resolve(mode_t reference, mode_t mask, const Vector<Who>& who_list) const;
        };

        Clause(Vector<Who>&& who_list, Vector<Action>&& action_list) : m_who_list(move(who_list)), m_action_list(move(action_list)) {}

        Vector<Who>& who_list() { return m_who_list; }
        const Vector<Who>& who_list() const { return m_who_list; }

        Vector<Action>& action_list() { return m_action_list; }
        const Vector<Action>& action_list() const { return m_action_list; }

    private:
        Vector<Who> m_who_list;
        Vector<Action> m_action_list;
    };

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

    mode_t resolve(mode_t reference) const;

private:
    Variant<mode_t, SymbolicMode> m_mode;
};

Maybe<Mode> parse_mode(const String& string);

}