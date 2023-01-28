#include <edit/document.h>
#include <edit/suggestions.h>
#include <edit/text_index.h>
#include <strings.h>

namespace Edit {
void Suggestions::do_match(const Suggestion& suggestion, StringView reference_text) {
    auto detailed_match = Vector<size_t> {};
    auto& suggestion_text = suggestion.content();

    int score = 0;
    int last_match = -1;
    size_t ri = 0;
    for (size_t si = 0; si < suggestion_text.size() && ri < reference_text.size(); si++) {
        if (toupper(reference_text[ri]) == toupper(suggestion_text[si])) {
            detailed_match.add(si);
            ri++;
            score += si - (last_match + 1);
            last_match = si;
        }
    }

    // Ensure the entire reference text appears in the suggestion.
    if (ri != reference_text.size()) {
        return;
    }

    m_matched_suggestions.add(MatchedSuggestion { suggestion, move(detailed_match), score });
}

void Suggestions::compute_matches(const Document& document, const Cursor& cursor) {
    for (auto& suggestion : m_suggestions) {
        auto reference_text = document.text_in_range({ suggestion.start(), cursor.index() });
        do_match(suggestion, reference_text.view());
    }

    Alg::sort(m_matched_suggestions, [](auto& a, auto& b) {
        if (auto result = Tuple { a.score(), a.content().size() } <=> Tuple { b.score(), b.content().size() }; result != 0) {
            return result;
        }
        return strcasecmp(a.content().string(), b.content().string()) <=> 0;
    });
}
}
