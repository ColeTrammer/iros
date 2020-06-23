#include <errno.h>
#include <ext/parse_mode.h>
#include <parser/generic_lexer.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "generic_mode_parser.h"
#include "mode_token_type.h"
#include "mode_value.h"

namespace Ext {

class ModeLexer final : public GenericLexer<ModeTokenType, ModeValue> {
public:
    using Token = GenericToken<ModeTokenType, ModeValue>;

    ModeLexer(const char* input_stream, size_t length)
        : GenericLexer<ModeTokenType, ModeValue>(), m_input_stream(input_stream), m_input_length(length) {}
    virtual ~ModeLexer() {}

    bool lex() {
        for (size_t i = 0; i < m_input_length; i++) {
            switch (m_input_stream[i]) {
                case 'a':
                    m_tokens.add({ ModeTokenType::LowerCaseA, {} });
                    break;
                case 'g':
                    m_tokens.add({ ModeTokenType::LowerCaseG, {} });
                    break;
                case 'o':
                    m_tokens.add({ ModeTokenType::LowerCaseO, {} });
                    break;
                case 'r':
                    m_tokens.add({ ModeTokenType::LowerCaseR, {} });
                    break;
                case 's':
                    m_tokens.add({ ModeTokenType::LowerCaseS, {} });
                    break;
                case 't':
                    m_tokens.add({ ModeTokenType::LowerCaseT, {} });
                    break;
                case 'u':
                    m_tokens.add({ ModeTokenType::LowerCaseU, {} });
                    break;
                case 'w':
                    m_tokens.add({ ModeTokenType::LowerCaseW, {} });
                    break;
                case 'x':
                    m_tokens.add({ ModeTokenType::LowerCaseX, {} });
                    break;
                case 'X':
                    m_tokens.add({ ModeTokenType::UpperCaseX, {} });
                    break;
                case '+':
                    m_tokens.add({ ModeTokenType::Plus, {} });
                    break;
                case '-':
                    m_tokens.add({ ModeTokenType::Minus, {} });
                    break;
                case '=':
                    m_tokens.add({ ModeTokenType::Equal, {} });
                    break;
                case ',':
                    m_tokens.add({ ModeTokenType::Comma, {} });
                    break;
                default:
                    return false;
            }
        }

        return true;
    }

    const Vector<Token>& tokens() const { return m_tokens; }

    virtual ModeTokenType peek_next_token_type() const override {
        if (m_current_pos >= (size_t) m_tokens.size()) {
            return ModeTokenType::End;
        }

        return m_tokens[m_current_pos].type();
    }

    virtual const ModeValue& peek_next_token_value() const override {
        assert(m_current_pos < (size_t) m_tokens.size());
        return m_tokens[m_current_pos].value();
    }

    virtual void advance() override { m_current_pos++; }

private:
    const char* m_input_stream { nullptr };
    size_t m_input_length { 0 };
    size_t m_position { 0 };
    size_t m_current_pos { 0 };
    Vector<Token> m_tokens;
};

class ModeParser final : public GenericModeParser {
public:
    using Token = GenericToken<ModeTokenType, ModeValue>;

    ModeParser(GenericLexer<ModeTokenType, ModeValue>& lexer) : GenericModeParser(lexer) {}

    virtual void on_error(ModeTokenType) override {}

    const SymbolicMode& result() {
        auto& last_value = this->peek_value_stack();
        assert(last_value.is<SymbolicMode>());
        return last_value.as<SymbolicMode>();
    }

    virtual ModeValue reduce_who$lowercasea(ModeValue&) override { return { Who::All }; }
    virtual ModeValue reduce_who$lowercaseg(ModeValue&) override { return { Who::Group }; }
    virtual ModeValue reduce_who$lowercaseo(ModeValue&) override { return { Who::Other }; }
    virtual ModeValue reduce_who$lowercaseu(ModeValue&) override { return { Who::User }; }

    virtual ModeValue reduce_wholist$who(ModeValue& value) override {
        Vector<Who> who_list;
        who_list.add(value.as<Who>());
        return { move(who_list) };
    }

    virtual ModeValue reduce_wholist$wholist_who(ModeValue& list, ModeValue& who) override {
        list.as<Vector<Who>>().add(who.as<Who>());
        return list;
    }

    virtual ModeValue reduce_modifier$minus(ModeValue&) override { return { Modifier::Minus }; }
    virtual ModeValue reduce_modifier$plus(ModeValue&) override { return { Modifier::Plus }; }
    virtual ModeValue reduce_modifier$equal(ModeValue&) override { return { Modifier::Equal }; }

    virtual ModeValue reduce_permission$lowercaser(ModeValue&) override { return { Permission::Read }; }
    virtual ModeValue reduce_permission$lowercases(ModeValue&) override { return { Permission::SetID }; }
    virtual ModeValue reduce_permission$lowercaset(ModeValue&) override { return { Permission::Sticky }; }
    virtual ModeValue reduce_permission$lowercasew(ModeValue&) override { return { Permission::Write }; }
    virtual ModeValue reduce_permission$lowercasex(ModeValue&) override { return { Permission::Execute }; }
    virtual ModeValue reduce_permission$uppercasex(ModeValue&) override { return { Permission::Search }; }

    virtual ModeValue reduce_permlist$permission(ModeValue& value) override {
        Permlist perms;
        perms.add(value.as<Permission>());
        return { move(perms) };
    }

    virtual ModeValue reduce_permlist$permission_permlist(ModeValue& value, ModeValue& list) override {
        list.as<Permlist>().insert(value.as<Permission>(), 0);
        return list;
    }

    virtual ModeValue reduce_permission_copy$lowercaseg(ModeValue&) override { return { PermissionCopy::Group }; }
    virtual ModeValue reduce_permission_copy$lowercaseo(ModeValue&) override { return { PermissionCopy::Other }; }
    virtual ModeValue reduce_permission_copy$lowercaseu(ModeValue&) override { return { PermissionCopy::User }; }

    virtual ModeValue reduce_action$modifier(ModeValue& op) override { return { Action { op.as<Modifier>(), {} } }; }

    virtual ModeValue reduce_action$modifier_permlist(ModeValue& op, ModeValue& permlist) override {
        return { Action { op.as<Modifier>(), { permlist.as<Vector<Permission>>() } } };
    }

    virtual ModeValue reduce_action$modifier_permission_copy(ModeValue& op, ModeValue& permlist) override {
        return { Action { op.as<Modifier>(), { permlist.as<PermissionCopy>() } } };
    }

    virtual ModeValue reduce_actionlist$action(ModeValue& value) override {
        Actionlist actions;
        actions.add(value.as<Action>());
        return { move(actions) };
    }

    virtual ModeValue reduce_actionlist$actionlist_action(ModeValue& list, ModeValue& value) override {
        list.as<Actionlist>().add(value.as<Action>());
        return list;
    }

    virtual ModeValue reduce_clause$actionlist(ModeValue& list) override {
        return { Clause { Vector<Who>(), move(list.as<Actionlist>()) } };
    }

    virtual ModeValue reduce_clause$wholist_actionlist(ModeValue& wlist, ModeValue& list) override {
        return { Clause { move(wlist.as<Vector<Who>>()), move(list.as<Actionlist>()) } };
    }

    virtual ModeValue reduce_symbolic_mode$clause(ModeValue& value) override {
        SymbolicMode mode;
        mode.clauses().add(value.as<Clause>());
        return mode;
    }

    virtual ModeValue reduce_symbolic_mode$symbolic_mode_comma_clause(ModeValue& mode, ModeValue&, ModeValue& value) override {
        mode.as<SymbolicMode>().clauses().add(value.as<Clause>());
        return mode;
    }
};

Maybe<Mode> parse_mode(const String& string) {
    if (string.is_empty()) {
        return {};
    }

    char* end_ptr;
    errno = 0;
    unsigned long octal_mode = strtoul(string.string(), &end_ptr, 8);
    if (!*end_ptr && errno == 0) {
        return { (mode_t) octal_mode };
    }

    ModeLexer lexer(string.string(), string.size());
    if (!lexer.lex()) {
        return {};
    }

    ModeParser parser(lexer);
    if (!parser.parse()) {
        return {};
    }

    return { { move(parser.result()) } };
}

mode_t Action::resolve(mode_t reference, mode_t mask, const Wholist& who_list) const {
    mode_t computed_mask = mask;
    if (!who_list.empty()) {
        computed_mask = 0;
    }

    for (auto who : who_list) {
        if (who == Who::All) {
            computed_mask = 0777;
            break;
        }
        computed_mask |= (7U << ((mode_t) who) * 3);
    }

    auto resolve_permission_copy = [&](PermissionCopy copy) -> mode_t {
        int bits_to_shift = ((int) copy) * 3;
        mode_t part_mask = 7 << bits_to_shift;
        mode_t max_perm = (reference & part_mask) >> bits_to_shift;
        max_perm |= max_perm << 3;
        max_perm |= max_perm << 3;

        return max_perm & computed_mask;
    };

    auto resolve_permission = [&](Permission perm) -> mode_t {
        switch (perm) {
            case Permission::Read:
            case Permission::Write:
            case Permission::Execute: {
                mode_t max_perm = (mode_t) perm;
                max_perm |= max_perm << 3;
                max_perm |= max_perm << 3;
                return max_perm & computed_mask;
            }
            case Permission::SetID: {
                if (who_list.empty()) {
                    return S_ISUID | S_ISGID;
                }

                mode_t ret = 0;
                if (computed_mask & 0700) {
                    ret |= S_ISUID;
                }
                if (computed_mask & 0070) {
                    ret |= S_ISGID;
                }
                return ret;
            }
            case Permission::Sticky:
                if (who_list.empty() || (who_list.size() == 1 && who_list.first() == Who::All)) {
                    return S_ISVTX;
                }
                return 0;
            case Permission::Search: {
                if ((reference & S_IFDIR) || (reference & 0111)) {
                    return 0111 & computed_mask;
                }
                return 0;
            }
        }

        return 0;
    };

    switch (this->modifier) {
        case Modifier::Plus:
        plus : {
            if (this->copy_or_permission_list.is<ModeLiteral>()) {
                return reference;
            }

            if (this->copy_or_permission_list.is<PermissionCopy>()) {
                reference |= resolve_permission_copy(this->copy_or_permission_list.as<PermissionCopy>());
                return reference;
            }

            auto& perm_list = this->copy_or_permission_list.as<Permlist>();
            for (auto perm : perm_list) {
                reference |= resolve_permission(perm);
            }

            return reference;
        }
        case Modifier::Minus: {
            if (this->copy_or_permission_list.is<Monostate>()) {
                return reference;
            }

            if (this->copy_or_permission_list.is<PermissionCopy>()) {
                reference &= ~(resolve_permission_copy(this->copy_or_permission_list.as<PermissionCopy>()));
                return reference;
            }

            auto& perm_list = this->copy_or_permission_list.as<Permlist>();
            for (auto perm : perm_list) {
                reference &= ~(resolve_permission(perm));
            }

            return reference;
        }
        case Modifier::Equal: {
            if (who_list.empty()) {
                reference = 0;
            } else {
                reference &= ~computed_mask;
            }
            reference &= ~07000;

            goto plus;
        }
    }

    return reference;
}

mode_t SymbolicMode::resolve(mode_t reference, mode_t mask) const {
    for (auto& clause : clauses()) {
        auto& who_list = clause.who_list();
        for (auto& action : clause.action_list()) {
            reference = action.resolve(reference, mask, who_list);
        }
    }
    return reference;
}

mode_t Mode::resolve(mode_t reference, Maybe<mode_t> umask_value) const {
    if (impl().is<mode_t>()) {
        return impl().as<mode_t>() & 07777;
    }

    if (!umask_value.has_value()) {
        mode_t mask = umask(0);
        umask(mask);
        umask_value = mask;
    }

    return impl().as<SymbolicMode>().resolve(reference, ~umask_value.value() & 0777) & 07777;
}

}
