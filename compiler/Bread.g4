grammar Bread;

prog:   (funcs=func)+;

func:   prot LBRACE (expr ';')+ RBRACE;

prot:   name=SYMNAME LBRACK argl RBRACK;

argl:   SYMNAME*;

expr:   left=expr '/' right=expr # div
    |   left=expr '*' right=expr # mul
    |   left=expr '+' right=expr # add
    |   left=expr '-' right=expr # sub
    |   name=SYMNAME LBRACK (args=expr)* RBRACK # call
    |   INT # lit
    |   '(' expr ')' # brack
    |   RETURN expr # ret
    ;

WS: [ \t\r\n]+ -> skip ;
RETURN  : 'return';
NEWLINE : [\r\n]+ ;
INT     : [0-9]+ ;
LBRACE  : '{';
RBRACE  : '}';
LBRACK  : '(';
RBRACK  : ')';
SYMNAME : [a-z]+;