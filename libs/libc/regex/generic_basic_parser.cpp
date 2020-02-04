#include "generic_basic_parser_impl.cpp"

#include "bre_value.h"

template bool GenericBasicParser<BREValue>::is_valid_token_type_in_current_state_for_shift(BasicTokenType type) const;
template bool GenericBasicParser<BREValue>::is_valid_token_type_in_current_state(BasicTokenType type) const;
template bool GenericBasicParser<BREValue>::parse();