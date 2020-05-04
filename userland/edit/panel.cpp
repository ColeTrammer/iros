#include "panel.h"
#include "editor.h"

Panel::~Panel() {}

void Panel::set_document(UniquePtr<Document> document) {
    if (m_document == document) {
        return;
    }

    m_document = move(document);
}