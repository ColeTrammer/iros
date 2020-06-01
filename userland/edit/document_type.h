#pragma once

namespace LIIM {
class StringView;
class String;
}

class Document;

enum class DocumentType {
    Text,
    C,
    CPP,
    ShellScript,
    Makefile,
};

void update_document_type(Document& document);

LIIM::String document_type_to_string(DocumentType type);

void highlight_document(Document& document);
