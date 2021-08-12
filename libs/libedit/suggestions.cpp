#include <edit/document.h>
#include <edit/suggestions.h>
#include <edit/text_index.h>

namespace Edit {
void Suggestions::do_match(const Suggestion& suggestion, StringView reference_text) {
    auto detailed_match = Vector<size_t> {};
    auto& suggestion_text = suggestion.content();

    size_t ri = 0;
    for (size_t si = 0; si < suggestion_text.size() && ri < reference_text.size(); si++) {
        if (reference_text[ri] == suggestion_text[si]) {
            detailed_match.add(si);
            ri++;
        }
    }

    // Ensure the entire reference text appears in the suggestion.
    if (ri != reference_text.size()) {
        return;
    }

    m_matched_suggestions.add(MatchedSuggestion { suggestion, move(detailed_match) });
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
