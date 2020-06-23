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

    virtual Who reduce_who$lowercasea(ModeTerminal&) override { return Who::All; }
    virtual Who reduce_who$lowercaseg(ModeTerminal&) override { return Who::Group; }
    virtual Who reduce_who$lowercaseo(ModeTerminal&) override { return Who::Other; }
    virtual Who reduce_who$lowercaseu(ModeTerminal&) override { return Who::User; }

    virtual Wholist reduce_wholist$who(Who& value) override { return Wholist::create_from_single_element(value); }

    virtual Wholist reduce_wholist$wholist_who(Wholist& list, Who& who) override {
        list.add(who);
        return list;
    }

    virtual Modifier reduce_modifier$minus(ModeTerminal&) override { return Modifier::Minus; }
    virtual Modifier reduce_modifier$plus(ModeTerminal&) override { return Modifier::Plus; }
    virtual Modifier reduce_modifier$equal(ModeTerminal&) override { return Modifier::Equal; }

    virtual Permission reduce_permission$lowercaser(ModeTerminal&) override { return Permission::Read; }
    virtual Permission reduce_permission$lowercases(ModeTerminal&) override { return Permission::SetID; }
    virtual Permission reduce_permission$lowercaset(ModeTerminal&) override { return Permission::Sticky; }
    virtual Permission reduce_permission$lowercasew(ModeTerminal&) override { return Permission::Write; }
    virtual Permission reduce_permission$lowercasex(ModeTerminal&) override { return Permission::Execute; }
    virtual Permission reduce_permission$uppercasex(ModeTerminal&) override { return Permission::Search; }

    virtual Permlist reduce_permlist$permission(Permission& value) override { return Permlist::create_from_single_element(move(value)); }

    virtual Permlist reduce_permlist$permission_permlist(Permission& value, Permlist& list) override {
        list.insert(move(value), 0);
        return list;
    }

    virtual PermissionCopy reduce_permission_copy$lowercaseg(ModeTerminal&) override { return PermissionCopy::Group; }
    virtual PermissionCopy reduce_permission_copy$lowercaseo(ModeTerminal&) override { return PermissionCopy::Other; }
    virtual PermissionCopy reduce_permission_copy$lowercaseu(ModeTerminal&) override { return PermissionCopy::User; }

    virtual Action reduce_action$modifier(Modifier& op) override { return { op, {} }; }
    virtual Action reduce_action$modifier_permlist(Modifier& op, Permlist& permlist) override { return { op, { move(permlist) } }; }

    virtual Action reduce_action$modifier_permission_copy(Modifier& op, PermissionCopy& permlist) override {
        return { op, { move(permlist) } };
    }

    virtual Actionlist reduce_actionlist$action(Action& value) override { return Actionlist::create_from_single_element(move(value)); }

    virtual Actionlist reduce_actionlist$actionlist_action(Actionlist& list, Action& value) override {
        list.add(move(value));
        return list;
    }

    virtual Clause reduce_clause$actionlist(Actionlist& list) override { return { Vector<Who>(), move(list) }; }
    virtual Clause reduce_clause$wholist_actionlist(Wholist& wlist, Actionlist& list) override { return { move(wlist), move(list) }; }

    virtual SymbolicMode reduce_symbolic_mode$clause(Clause& value) override {
        SymbolicMode mode;
        mode.clauses().add(move(value));
        return mode;
    }

    virtual SymbolicMode reduce_symbolic_mode$symbolic_mode_comma_clause(SymbolicMode& mode, ModeTerminal&, Clause& value) override {
        mode.clauses().add(move(value));
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
            if (this->copy_or_permission_list.is<ModeTerminal>()) {
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
