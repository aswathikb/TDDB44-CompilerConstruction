%{

#include <stdlib.h>
#include <string.h>

#if defined(LAB1)

#include "scanner.hh"

#elif defined(LAB2)

#include "scanner.hh"
#include "symtab.hh"

#else

#include "ast.hh"
#include "parser.hh"

#endif

int column = 0;

extern YYLTYPE yylloc; // Used for position information, see below.

%}

%option yylineno
%option 8bit
%option noyywrap
%option caseless
%option outfile = "scanner.cc"

/* If you want to include any flex declarations, this is where to do it. */

%x c_comment
%x comment
DIGIT [0-9]
/* {DIGIT}+"."{DIGIT}*
 ID [a-z][a-z0-9]* */
INTEGER {DIGIT}+
DECIMAL {DIGIT}+"."{DIGIT}*|"."{DIGIT}+
EXPONENT [eE][+-]?{INTEGER}
FLOAT {DECIMAL}{EXPONENT}?|{INTEGER}{EXPONENT}
NUMERICALCONSTANT {INTEGER}|{FLOAT}
STRING '([^'\n]|'')*'
ID [A-Za-z_][A-Za-z_0-9]*
SPACE [ \t]
COMMENT '{'[^'}']*'}'


/* Position information is entered into the yylloc variable, so we will be
   able to generate good error messages later in later phases. */
/* Your code should be entered below the %%. Expressions to handle the
   following: Diesel comments, Diesel string constants, Diesel
   identifiers, integers, reals, and whitespace. */
%%

\.                       {
                            yylloc.first_line = yylineno;
                            yylloc.first_column = column;
                            column += yyleng;
                            return T_DOT;
                         }
;                        {
                            yylloc.first_line = yylineno;
                            yylloc.first_column = column;
                            column += yyleng;
                            return T_SEMICOLON;
                         }
=                        {
                            yylloc.first_line = yylineno;
                            yylloc.first_column = column;
                            column += yyleng;
                            return T_EQ;
                         }
\:                       {
                            yylloc.first_line = yylineno;
                            yylloc.first_column = column;
                            column += yyleng;
                            return T_COLON;
                         }
\(                       {
                            yylloc.first_line = yylineno;
                            yylloc.first_column = column;
                            column += yyleng;
                            return T_LEFTPAR;
                         }
\)                       {
                            yylloc.first_line = yylineno;
                            yylloc.first_column = column;
                            column += yyleng;
                            return T_RIGHTPAR;
                         }
\[                       {
                            yylloc.first_line = yylineno;
                            yylloc.first_column = column;
                            column += yyleng;
                            return T_LEFTBRACKET;
                         }
\]                       {
                            yylloc.first_line = yylineno;
                            yylloc.first_column = column;
                            column += yyleng;
                            return T_RIGHTBRACKET;
                         }
,                        {
                            yylloc.first_line = yylineno;
                            yylloc.first_column = column;
                            column += yyleng;
                            return T_COMMA;
                         }
\<                       {
                            yylloc.first_line = yylineno;
                            yylloc.first_column = column;
                            column += yyleng;
                            return T_LESSTHAN;
                         }
\>                       {
                            yylloc.first_line = yylineno;
                            yylloc.first_column = column;
                            column += yyleng;
                            return T_GREATERTHAN;
                         }
\+                       {
                            yylloc.first_line = yylineno;
                            yylloc.first_column = column;
                            column += yyleng;
                            return T_ADD;
                         }
\-                       {
                            yylloc.first_line = yylineno;
                            yylloc.first_column = column;
                            column += yyleng;
                            return T_SUB;
                         }
\*                       {
                            yylloc.first_line = yylineno;
                            yylloc.first_column = column;
                            column += yyleng;
                            return T_MUL;
                         }
\/                       {
                            yylloc.first_line = yylineno;
                            yylloc.first_column = column;
                            column += yyleng;
                            return T_RDIV;
                         }
":="                     {
                            yylloc.first_line = yylineno;
                            yylloc.first_column = column;
                            column += yyleng;
                            return T_ASSIGN;
                         }
"<>"                     {
                            yylloc.first_line = yylineno;
                            yylloc.first_column = column;
                            column += yyleng;
                            return T_NOTEQ;
                         }


of                       {
                            
                         }
if                       {
                            
                         }
do                       {
                            
                         }
or                       {
                            
                         }
var                      {
                            
                         }
end                      {
                            
                         }
and                      {
                            
                         }
div                      {
                            
                         }
mod                      {
                            
                         }
not                      {
                            
                         }
then                     {
                            
                         }
else                     {
                            
                         }
const                    {
                            
                         }
array                    {
                            
                         }
begin                    {
                            
                         }
while                    {
                            
                         }
elsif                    {
                            
                         }
return                   {
                            
                         }
program                  {
                           yylloc.first_line = yylineno;
                           yylloc.first_column = column;
                           column += yyleng;
                           return T_PROGRAM;
                         }
function                 {
                            
                         }
procedure                {
                            
                         }

                     
{FLOAT}                  {
                           yylloc.first_line = yylineno;
                           yylloc.first_column = column;
                           column += yyleng;
                           yylval.rval = stod(yytext);
                           return T_REALNUM;
                         }
{SPACE}                  {
                           column++;
}
\n                       {
                           column = 0;
                         }
{INTEGER}                {
                           yylloc.first_line = yylineno;
                           yylloc.first_column = column;
                           column += yyleng;
                           yylval.ival = atoi(yytext);
                           return T_INTNUM;
                         }
"{"                      {
                            column += yyleng;
                            BEGIN(comment);
                         }
<comment>
{
    "}"                  {
                            column += 1;
                            BEGIN(INITIAL);
                         }
    [^\n]                column++; /* Skip stuff in comments */
    \n                   column = 0;
    <<EOF>>              {
                            yyerror("Unterminated comment");
                            yyterminate();
                         }
}             
{ID}                {
                           yylloc.first_line = yylineno;
                           yylloc.first_column = column;
                           yylval.pool_pos = column;
                           column += yyleng;
                           return T_IDENT;
                         }

\/\/.*$                  column = 0; /* Skip single-line comment */
"/\*"                    {
                            column += yyleng;
                            BEGIN(c_comment);
                         }

<c_comment>
{
    "\*/"                {
                            column += 2;
                            BEGIN(INITIAL);
                         }
    "/\*"                {
                            column += 2;
                            yyerror("Suspicious comment");
                         }
    [^\n]                column++; /* Skip stuff in comments */
    \n                   column = 0;
    <<EOF>>              {
                            yyerror("Unterminated comment");
                            yyterminate();
                         }
}

 

<<EOF>>                  yyterminate();
.                        yyerror("Illegal character");
