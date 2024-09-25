grammar Bread;

prog:   expr NEWLINE ;
expr:   left=expr '/' right=expr # div
    |   left=expr '*' right=expr # mul
    |   left=expr '+' right=expr # add
    |   left=expr '-' right=expr # sub
    |   INT # lit
    |   '(' expr ')' # brack
    ;
    
NEWLINE : [\r\n]+ ;
INT     : [0-9]+ ;