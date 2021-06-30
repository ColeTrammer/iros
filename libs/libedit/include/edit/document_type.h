#pragma once

#include <edit/forward.h>
#include <liim/forward.h>

namespace Edit {
enum class DocumentType {
    Text,
    C,
    CPP,
    ShellScript,
    Makefile,
};

void update_document_type(Document& document);

String document_type_to_string(DocumentType type);

void highlight_document(Document& document);
}
