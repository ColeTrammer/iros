#include <assert.h>
#include <liim/string.h>
#include <liim/string_view.h>

#include "document_type.h"

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
    if (view == "sh") {
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
