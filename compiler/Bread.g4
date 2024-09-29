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
    |   type=TYPE_ID name=SYMNAME # decl
    |   name=SYMNAME EQUAL expr # ass
    |   name=SYMNAME # var
    ;

WS: [ \t\r\n]+ -> skip;
TYPE_ID: 'i32';
RETURN  : 'return';
NEWLINE : [\r\n]+ ;
INT     : [0-9]+ ;
LBRACE  : '{';
RBRACE  : '}';
LBRACK  : '(';
RBRACK  : ')';
EQUAL   : '=';
SYMNAME : [a-z]+;
