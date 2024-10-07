grammar Bread;

prog:   (funcs=func)+;

func:   prot body;

prot:   name=SYMNAME LBRACK argl RBRACK;

argl:   SYMNAME*;

cbod:   LBRACE (stmt)+ RBRACE;
body:   LBRACE (stmt)+ RBRACE;

stmt:   IF LBRACK expr RBRACK bod=cbod # if
    |   RETURN expr ';' # ret
    |   expr ';' # exp
    ;

expr:   left=expr '/' right=expr # div
    |   left=expr '*' right=expr # mul
    |   left=expr '+' right=expr # add
    |   left=expr '-' right=expr # sub
    |   name=SYMNAME LBRACK (args=expr)* RBRACK # call
    |   INT # lit
    |   '(' expr ')' # brack
    |   type=TYPE_ID name=SYMNAME # decl
    |   name=SYMNAME EQUAL expr # ass
    |   name=SYMNAME # var
    |   left=expr EEQUAL right=expr # eq
    |   left=expr LT right=expr # lt
    |   left=expr GT right=expr # gt
    ;

WS: [ \t\r\n]+ -> skip;
TYPE_ID : 'i32';
IF      : 'if'; 
RETURN  : 'return';
NEWLINE : [\r\n]+ ;
INT     : [0-9]+ ;
LBRACE  : '{';
RBRACE  : '}';
LBRACK  : '(';
RBRACK  : ')';
EQUAL   : '=';
EEQUAL  : '==';
LT      : '<';
GT      : '>';
SYMNAME : [a-z]+;
