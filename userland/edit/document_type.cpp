#include <assert.h>
#include <liim/string.h>
#include <liim/string_view.h>
#include <sh/sh_lexer.h>

#include "character_metadata.h"
#include "document.h"
#include "document_type.h"
#include "line.h"

DocumentType document_type_from_extension(const LIIM::StringView& view) {
    if (view == "c") {
        return DocumentType::C;
    }
    if (view == "cpp") {
        return DocumentType::CPP;
    }
    if (view == "h") {
        // FIXME: this should sometimes return C, but CPP mode will hopefully work fine with most C headers.
        return DocumentType::CPP;
    }
    if (view == "sh" || view == "sh_init" || view == "bashrc") {
        return DocumentType::ShellScript;
    }
    return DocumentType::Text;
}

LIIM::String document_type_to_string(DocumentType type) {
    switch (type) {
        case DocumentType::C:
            return "C";
        case DocumentType::CPP:
            return "C++";
        case DocumentType::ShellScript:
            return "Shell Script";
        case DocumentType::Text:
            return "Text";
        default:
            assert(false);
    }
}

int sh_flags_for_token_type(ShTokenType type) {
    switch (type) {
        case ShTokenType::Ampersand:
        case ShTokenType::Bang:
        case ShTokenType::AND_IF:
        case ShTokenType::CLOBBER:
        case ShTokenType::DGREAT:
        case ShTokenType::DLESS:
        case ShTokenType::DLESSDASH:
        case ShTokenType::DSEMI:
        case ShTokenType::GREATAND:
        case ShTokenType::Lbrace:
        case ShTokenType::LeftParenthesis:
        case ShTokenType::LESSAND:
        case ShTokenType::LESSGREAT:
        case ShTokenType::LessThan:
        case ShTokenType::OR_IF:
        case ShTokenType::Pipe:
        case ShTokenType::Rbrace:
        case ShTokenType::RightParenthesis:
        case ShTokenType::Semicolon:
        case ShTokenType::TLESS:
            return CharacterMetadata::Flags::SyntaxOperator;
        case ShTokenType::NAME:
            return CharacterMetadata::Flags::SyntaxIdentifier;
        case ShTokenType::IO_NUMBER:
            return CharacterMetadata::Flags::SyntaxNumber;
        case ShTokenType::COMMENT:
            return CharacterMetadata::Flags::SyntaxComment;
        case ShTokenType::Case:
        case ShTokenType::Do:
        case ShTokenType::Done:
        case ShTokenType::Elif:
        case ShTokenType::Else:
        case ShTokenType::Esac:
        case ShTokenType::Fi:
        case ShTokenType::For:
        case ShTokenType::If:
        case ShTokenType::In:
        case ShTokenType::Then:
        case ShTokenType::Until:
        case ShTokenType::While:
            return CharacterMetadata::Flags::SyntaxKeyword;
        case ShTokenType::ASSIGNMENT_WORD:
        case ShTokenType::WORD:
        default:
            return 0;
    }
}

static void highlight_sh(Document& document) {
    auto contents = document.content_string();
    ShLexer lexer(contents.string(), contents.size());
    if (!lexer.lex(LexComments::Yes)) {
        return;
    }

    if (lexer.tokens().empty()) {
        return;
    }

    int line_index = 0;
    int index_into_line = 0;
    int flags = sh_flags_for_token_type(lexer.tokens()[0].type());
    for (int i = 1; i <= lexer.tokens().size(); i++) {
        int end_line = document.num_lines() - 1;
        int end_position = document.last_line().length();
        if (i != lexer.tokens().size()) {
            auto& token = lexer.tokens()[i];
            end_line = token.value().line();
            end_position = token.value().position();
        }

        while ((line_index < document.num_lines()) &&
               (line_index < end_line || (index_into_line < end_position && line_index == end_line))) {
            auto& line = document.line_at_index(line_index);
            if (line.empty()) {
                line_index++;
                continue;
            }

            line.metadata_at(index_into_line).set_syntax_highlighting(flags);
            if (index_into_line == line.length() - 1) {
                index_into_line = 0;
                line_index++;
            } else {
                index_into_line++;
            }
        }

        if (i != lexer.tokens().size()) {
            flags = sh_flags_for_token_type(lexer.tokens()[i].type());
        }
    }
}

void highlight_document(Document& document) {
    switch (document.type()) {
        case DocumentType::ShellScript:
            highlight_sh(document);
            return;
        default:
            return;
    }
}
