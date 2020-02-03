%token    OrdinaryCharacter QuotedCharacter DuplicateCount
%token    BackReference LeftAnchor RightAnchor
%token    CollateSingleElement CollateMultipleElements MetaCharacter
%token    ClassName

%start    basic_reg_exp
%%
basic_reg_exp            :            RE_expression
                         | LeftAnchor
                         |                          RightAnchor
                         | LeftAnchor               RightAnchor
                         | LeftAnchor RE_expression
                         |            RE_expression RightAnchor
                         | LeftAnchor RE_expression RightAnchor
                         ;
RE_expression            :               simple_RE
                         | RE_expression simple_RE
                         ;
simple_RE                : nondupl_RE
                         | nondupl_RE RE_dupl_symbol
                         ;
nondupl_RE               : one_char_or_coll_elem_RE
                         | '\(' RE_expression '\)'
                         | BackReference
                         ;
one_char_or_coll_elem_RE : OrdinaryCharacter
                         | QuotedCharacter
                         | '.'
                         | bracket_expression
                         ;
RE_dupl_symbol           : '*'
                         | '\{' DuplicateCount                    '\}'
                         | '\{' DuplicateCount ','                '\}'
                         | '\{' DuplicateCount ',' DuplicateCount '\}'
                         ;


/* --------------------------------------------
   Bracket Expression
   -------------------------------------------
*/
bracket_expression : '[' matching_list ']'
                   | '[' nonmatching_list ']'
                   ;
matching_list      : bracket_list
                   ;
nonmatching_list   : '^' bracket_list
                   ;
bracket_list       : follow_list
                   | follow_list '-'
                   ;
follow_list        :             expression_term
                   | follow_list expression_term
                   ;
expression_term    : single_expression
                   | range_expression
                   ;
single_expression  : end_range
                   | character_class
                   | equivalence_class
                   ;
range_expression   : start_range end_range
                   | start_range '-'
                   ;
start_range        : end_range '-'
                   ;
end_range          : CollateSingleElement
                   | collating_symbol
                   ;
collating_symbol   : '[.' CollateSingleElement    '.]'
                   | '[.' CollateMultipleElements '.]'
                   | '[.' MetaCharacter           '.]'
                   ;
equivalence_class  : '[=' CollateSingleElement    '=]'
                   | '[=' CollateMultipleElements '=]'
                   ;
character_class    : '[:' ClassName ':]'
                   ;