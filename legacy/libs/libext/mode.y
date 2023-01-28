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


action           : modifier
                 | modifier permlist
                 | modifier permission_copy
                 ;


permission_copy  : 'u' | 'g' | 'o'
                 ;


modifier         : '+' | '-' | '='
                 ;


permlist         : permission
                 | permission permlist
                 ;


permission       : 'r' | 'w' | 'x' | 'X' | 's' | 't'  
                 ;
