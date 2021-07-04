#include <assert.h>
#include <clanguage/clexer.h>
#include <clanguage/cpplexer.h>
#include <edit/character_metadata.h>
#include <edit/document.h>
#include <edit/document_type.h>
#include <edit/line.h>
#include <liim/string.h>
#include <liim/string_view.h>
#include <sh/sh_lexer.h>
#include <stdlib.h>
#include <string.h>

namespace Edit {
void update_document_type(Document& document) {
    auto path = document.name();

    auto filename = StringView(path.string(), path.string() + path.size() - 1);
    if (char* last_slash = strrchr(path.string(), '/')) {
        filename = StringView(last_slash + 1, path.string() + path.size() - 1);
    }

    auto extension = StringView("");
    if (const char* last_dot = strrchr(filename.start(), '.')) {
        extension = StringView(last_dot + 1, filename.end());
    }

    DocumentType type;
    if (extension == "c") {
        type = DocumentType::C;
    } else if (extension == "cpp") {
        type = DocumentType::CPP;
    } else if (extension == "h") {
        // FIXME: this should sometimes return C, but CPP mode will hopefully work fine with most C headers.
        type = DocumentType::CPP;
    } else if (extension == "sh" || filename == ".sh_init" || filename == ".bashrc") {
        type = DocumentType::ShellScript;
    } else if (filename == "Makefile" || extension == "d" || extension == "config") {
        type = DocumentType::Makefile;
        document.set_convert_tabs_to_spaces(false);
    } else {
        type = DocumentType::Text;
    }

    document.set_type(type);
}

LIIM::String document_type_to_string(DocumentType type) {
    switch (type) {
        case DocumentType::C:
            return "C";
        case DocumentType::CPP:
            return "C++";
        case DocumentType::ShellScript:
            return "Shell Script";
        case DocumentType::Makefile:
            return "Makefile";
        case DocumentType::Text:
            return "Text";
        default:
            assert(false);
    }
}

static int cpp_flags_for_token_type(CLanguage::CPPToken::Type type) {
    using CLanguage::CPPToken;
    switch (type) {
        case CPPToken::Type::CharacterLiteral:
        case CPPToken::Type::StringLiteral:
        case CPPToken::Type::PreprocessorSystemIncludeString:
            return CharacterMetadata::Flags::SyntaxString;
#undef __ENUMERATE_CPP_LITERAL
#define __ENUMERATE_CPP_LITERAL(n, s) case CPPToken::Type::Literal##n:
            __ENUMERATE_CPP_LITERALS
        case CPPToken::Type::NumericLiteral:
            return CharacterMetadata::Flags::SyntaxNumber;
        case CPPToken::Type::Comment:
            return CharacterMetadata::Flags::SyntaxComment;

#undef __ENUMERATE_CPP_TYPE_TOKEN
#define __ENUMERATE_CPP_TYPE_TOKEN(n, s) \
    case CPPToken::Type::Type##n:        \
        return CharacterMetadata::Flags::SyntaxKeyword;
            __ENUMERATE_CPP_TYPE_TOKENS

#undef __ENUMERATE_CPP_KEYWORD
#define __ENUMERATE_CPP_KEYWORD(n, s) \
    case CPPToken::Type::Keyword##n:  \
        return CharacterMetadata::Flags::SyntaxKeyword;
            __ENUMERATE_CPP_KEYWORDS

#undef __ENUMERATE_CPP_OP
#define __ENUMERATE_CPP_OP(n, s)      \
    case CPPToken::Type::Operator##n: \
        return CharacterMetadata::Flags::SyntaxOperator;
            __ENUMERATE_CPP_OPS

#undef __ENUMERATE_CPP_PREPROCESSOR_KEYWORD
#define __ENUMERATE_CPP_PREPROCESSOR_KEYWORD(n, s) \
    case CPPToken::Type::Preprocessor##n:          \
        return CharacterMetadata::Flags::SyntaxOperator;
            __ENUMERATE_CPP_PREPROCESSOR_KEYWORDS

        case CPPToken::Type::PreprocessorBackslash:
        case CPPToken::Type::PreprocessorDefined:
        case CPPToken::Type::PreprocessorStart:
        case CPPToken::Type::PreprocessorPound:
        case CPPToken::Type::PreprocessorPoundPound:
            return CharacterMetadata::Flags::SyntaxOperator;

        case CPPToken::Type::Identifier:
        default:
            return 0;
    }
}

static void highlight_cpp(Document& document) {
    auto& range_collection = document.syntax_highlighting_info();
    range_collection.clear();

    auto contents = document.content_string();
    CLanguage::CPPLexer lexer(contents.string(), contents.size());
    lexer.lex(CLanguage::CPPLexMode::IncludeComments);

    for (auto& token : lexer.tokens()) {
        int flags = cpp_flags_for_token_type(token.type);
        range_collection.add({ token.start_line, token.start_col, token.end_line, token.end_col - 1, { flags } });
    }
}

static int c_flags_for_token_type(CLanguage::CToken::Type type) {
    using CLanguage::CToken;
    switch (type) {
        case CToken::Type::CharacterLiteral:
        case CToken::Type::StringLiteral:
        case CToken::Type::PreprocessorSystemIncludeString:
            return CharacterMetadata::Flags::SyntaxString;
        case CToken::Type::NumericLiteral:
            return CharacterMetadata::Flags::SyntaxNumber;
        case CToken::Type::Comment:
            return CharacterMetadata::Flags::SyntaxComment;

#undef __ENUMERATE_C_KEYWORD
#define __ENUMERATE_C_KEYWORD(n, s) \
    case CToken::Type::Keyword##n:  \
        return CharacterMetadata::Flags::SyntaxKeyword;
            __ENUMERATE_C_KEYWORDS

#undef __ENUMERATE_C_OP
#define __ENUMERATE_C_OP(n, s)      \
    case CToken::Type::Operator##n: \
        return CharacterMetadata::Flags::SyntaxOperator;
            __ENUMERATE_C_OPS

#undef __ENUMERATE_C_PREPROCESSOR_KEYWORD
#define __ENUMERATE_C_PREPROCESSOR_KEYWORD(n, s) \
    case CToken::Type::Preprocessor##n:          \
        return CharacterMetadata::Flags::SyntaxOperator;
            __ENUMERATE_C_PREPROCESSOR_KEYWORDS

        case CToken::Type::PreprocessorBackslash:
        case CToken::Type::PreprocessorDefined:
        case CToken::Type::PreprocessorStart:
        case CToken::Type::PreprocessorPound:
        case CToken::Type::PreprocessorPoundPound:
            return CharacterMetadata::Flags::SyntaxOperator;

        case CToken::Type::Identifier:
        default:
            return 0;
    }
}

static void highlight_c(Document& document) {
    auto& range_collection = document.syntax_highlighting_info();
    range_collection.clear();

    auto contents = document.content_string();
    CLanguage::CLexer lexer(contents.string(), contents.size());
    lexer.lex(CLanguage::CLexMode::IncludeComments);

    for (auto& token : lexer.tokens()) {
        int flags = c_flags_for_token_type(token.type);
        range_collection.add({ token.start_line, token.start_col, token.end_line, token.end_col, { flags } });
    }
}

static int sh_flags_for_token_type(const ShLexer& lexer, int index) {
    auto type = lexer.tokens()[index].type();
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
        case ShTokenType::GreaterThan:
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
        case ShTokenType::HERE_END:
            return CharacterMetadata::Flags::SyntaxString;
        case ShTokenType::WORD:
            if (lexer.would_be_first_word_of_command(index)) {
                return CharacterMetadata::Flags::SyntaxImportant;
            }
            return 0;
        case ShTokenType::ASSIGNMENT_WORD:
        case ShTokenType::NEWLINE:
        default:
            return 0;
    }
}

static void highlight_sh(Document& document) {
    auto& range_collection = document.syntax_highlighting_info();
    range_collection.clear();

    auto contents = document.content_string();
    ShLexer lexer(contents.string(), contents.size());
    lexer.lex(LexComments::Yes);

    if (lexer.tokens().empty()) {
        return;
    }

    auto do_sort = [](const void* a, const void* b) -> int {
        auto t1 = reinterpret_cast<const ShLexer::Token*>(a);
        auto t2 = reinterpret_cast<const ShLexer::Token*>(b);

        assert(t1->value().has_text());
        assert(t2->value().has_text());

        if (t1->value().start_line() < t2->value().start_line()) {
            return -1;
        } else if (t1->value().start_line() > t2->value().start_line()) {
            return 1;
        }
        return t1->value().start_col() - t2->value().start_col();
    };

    while (lexer.peek_next_token_type() != ShTokenType::End) {
        lexer.advance();
    }

    qsort(const_cast<ShLexer::Token*>(lexer.tokens().vector()), lexer.tokens().size(), sizeof(ShLexer::Token), do_sort);

    for (int i = 0; i < lexer.tokens().size(); i++) {
        int flags = sh_flags_for_token_type(lexer, i);

        auto& token_value = lexer.tokens()[i].value();
        range_collection.add({ static_cast<int>(token_value.start_line()),
                               static_cast<int>(token_value.start_col()),
                               static_cast<int>(token_value.end_line()),
                               static_cast<int>(token_value.end_col() - 1),
                               { flags } });
    }
}

void highlight_document(Document& document) {
    switch (document.type()) {
        case DocumentType::CPP:
            highlight_cpp(document);
            return;
        case DocumentType::C:
            highlight_c(document);
            return;
        case DocumentType::ShellScript:
            highlight_sh(document);
            return;
        default:
            return;
    }
}
}
