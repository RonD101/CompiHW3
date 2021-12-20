%{
    #include <stdio.h>
    #include "SemanticAnalyzer.h"
    #include "hw3_output.hpp"
    #include "parser.tab.hpp"
%}

%option yylineno
%option noyywrap
digit  ([0-9])
letter ([a-zA-Z])
ws     ([\t\n ])
%%

void                          yylval = new BaseType(yytext); return VOID; 
int                           yylval = new BaseType(yytext); return INT;
byte                          yylval = new BaseType(yytext); return BYTE;
b                             yylval = new BaseType(yytext); return B;
bool                          yylval = new BaseType(yytext); return BOOL;
const                         yylval = new BaseType(yytext); return CONST;
and                           yylval = new BaseType(yytext); return AND;
or                            yylval = new BaseType(yytext); return OR;
not                           yylval = new BaseType(yytext); return NOT;
true                          yylval = new BaseType(yytext); return TRUE;
false                         yylval = new BaseType(yytext); return FALSE;
return                        yylval = new BaseType(yytext); return RETURN;
if                            yylval = new BaseType(yytext); return IF;
else                          yylval = new BaseType(yytext); return ELSE;
while                         yylval = new BaseType(yytext); return WHILE;
break                         yylval = new BaseType(yytext); return BREAK;
continue                      yylval = new BaseType(yytext); return CONTINUE;
;                             yylval = new BaseType(yytext); return SC;
,                             yylval = new BaseType(yytext); return COMMA;
\(                            yylval = new BaseType(yytext); return LPAREN;
\)                            yylval = new BaseType(yytext); return RPAREN;
\{                            yylval = new BaseType(yytext); return LBRACE;
\}                            yylval = new BaseType(yytext); return RBRACE;
=                             yylval = new BaseType(yytext); return ASSIGN;
==|!=                         yylval = new BaseType(yytext); return EQUALITY;
\<|>|<=|>=                    yylval = new BaseType(yytext); return RELATION;
\+|-                          yylval = new BaseType(yytext); return BINADD;
\*|\/                         yylval = new BaseType(yytext); return BINMUL;
{letter}+({letter}|{digit})*  yylval = new BaseType(yytext); return ID;
([1-9]{digit}*)|0             yylval = new BaseType(yytext); return NUM;
\"([^\n\r\"\\]|\\[rnt"\\])+\" yylval = new BaseType(yytext); return STRING;
{ws}                          {};
(\r)|(\r\n)                   {};
\/\/[^\r\n]*[ \r|\n|\r\n]?    {};
.                             { output::errorLex(yylineno); };
%%
