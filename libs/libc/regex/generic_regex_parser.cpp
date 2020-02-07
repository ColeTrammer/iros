#include "generic_regex_parser_impl.cpp"

#include "regex_value.h"

template bool GenericRegexParser<RegexValue>::is_valid_token_type_in_current_state_for_shift(RegexTokenType type) const;
template bool GenericRegexParser<RegexValue>::is_valid_token_type_in_current_state(RegexTokenType type) const;
template bool GenericRegexParser<RegexValue>::parse();