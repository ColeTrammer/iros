%start    symbolic_mode
%%


symbolic_mode    : clause
                 | symbolic_mode ',' clause
                 ;


clause           : actionlist
                 | wholist actionlist
                 ;


wholist          : who
                 | wholist who
                 ;


who              : 'u' | 'g' | 'o' | 'a'
                 ;


actionlist       : action
                 | actionlist action
                 ;


action           : op
                 | op permlist
                 | op permcopy
                 ;


permcopy         : 'u' | 'g' | 'o'
                 ;


op               : '+' | '-' | '='
                 ;


permlist         : perm
                 | perm permlist
                 ;


perm             : 'r' | 'w' | 'x' | 'X' | 's' | 't'  
                 ;