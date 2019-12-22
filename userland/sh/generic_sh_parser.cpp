#include "generic_sh_parser_impl.cpp"

#include "sh_token.h"

template bool GenericShParser<ShValue>::is_valid_token_type_in_current_state(ShTokenType type);
template bool GenericShParser<ShValue>::parse();