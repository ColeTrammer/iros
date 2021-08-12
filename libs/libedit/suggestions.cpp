#include <edit/document.h>
#include <edit/suggestions.h>
#include <edit/text_index.h>

namespace Edit {
void Suggestions::do_match(const Suggestion& suggestion, StringView reference_text) {
    if (!suggestion.content().view().starts_with(reference_text)) {
        return;
    }

    m_matched_suggestions.add(MatchedSuggestion { suggestion });
}

void Suggestions::compute_matches(const Document& document, const Cursor& cursor) {
    auto content_string = document.content_string();
    auto cursor_byte_index = document.cursor_index_in_content_string(cursor);

    for (auto& suggestion : m_suggestions) {
        auto reference_text = content_string.view().substring(cursor_byte_index - suggestion.offset(), suggestion.offset());
        do_match(suggestion, reference_text);
    }
}
}
