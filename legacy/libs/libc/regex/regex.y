%token    OrdinaryCharacter QuotedCharacter DuplicateCount
%token    BackReference LeftAnchor RightAnchor
%token    CollateSingleElement CollateMultipleElements MetaCharacter
%token    ClassName

%start    regex
%%
regex              :                      regex_branch
                   | regex '|'            regex_branch
                   ;
regex_branch       :              expression
                   | regex_branch expression
                   ;
expression         : regex_one_char
                   | '^'
                   | '$'
                   | '(' regex ')'
                   | BackReference
                   | expression duplicate_symbol
                   ;
regex_one_char     : OrdinaryCharacter
                   | QuotedCharacter
                   | '.'
                   | bracket_expression
                   ;
duplicate_symbol   : '*'
                   | '+'
                   | '?'
                   | '{' DuplicateCount                    '}'
                   | '{' DuplicateCount ','                '}'
                   | '{' DuplicateCount ',' DuplicateCount '}'
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
