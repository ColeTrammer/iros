#include "panel.h"
#include "document.h"

Panel::Panel() {}

Panel::~Panel() {}

void Panel::set_document(UniquePtr<Document> document) {
    if (m_document == document) {
        return;
    }

    m_document = move(document);
    document_did_change();
}
