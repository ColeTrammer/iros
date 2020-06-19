#define GENERIC_MODE_PARSER_DEBUG

#include "generic_mode_parser_impl.cpp"

#include "mode_value.h"

template bool GenericModeParser<Ext::ModeValue>::is_valid_token_type_in_current_state_for_shift(ModeTokenType type) const;
template bool GenericModeParser<Ext::ModeValue>::is_valid_token_type_in_current_state(ModeTokenType type) const;
template bool GenericModeParser<Ext::ModeValue>::parse();
