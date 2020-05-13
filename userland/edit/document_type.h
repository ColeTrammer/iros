#pragma once

namespace LIIM {
class StringView;
class String;
}

enum class DocumentType {
    Text,
    C,
    CPP,
    ShellScript,
};

DocumentType document_type_from_extension(const LIIM::StringView& view);

LIIM::String document_type_to_string(DocumentType type);
