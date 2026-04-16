%{
#include "ast.h"
#include "parser.tab.hpp"
#include <string>
#include <cstdlib>

extern void yyerror(const char* s);
%}

%option noyywrap
%option yylineno

DIGIT       [0-9]
LETTER      [a-zA-Z_]
ALNUM       [a-zA-Z0-9_]
FLOAT       {DIGIT}+\.{DIGIT}*([eE][+-]?{DIGIT}+)?
INTEGER     {DIGIT}+
IDENTIFIER  {LETTER}{ALNUM}*
STRING      \"[^\"]*\"
COMMENT     \/\/.*$

%%

{COMMENT}                   { }

[ 	\r]+                    { }

"version"                   { return VERSION; }
"target"                    { return TARGET; }
"address_size"              { return ADDRESS_SIZE; }
"visible"                   { return VISIBLE; }
"entry"                     { return ENTRY; }
"globl"                     { return GLOBL; }
"param"                     { return PARAM; }
"reg"                       { return REG; }
"pred"                      { return PRED; }
"ret"                       { return RET; }

"ld"                        { return LD; }
"st"                        { return ST; }
"mov"                       { return MOV; }
"add"                       { return ADD; }
"sub"                       { return SUB; }
"mul"                       { return MUL; }
"div"                       { return DIV; }
"mad"                       { return MAD; }
"fma"                       { return FMA; }
"cvta"                      { return CVTA; }
"setp"                      { return SETP; }
"bra"                       { return BRA; }

"u8"|"s8"                   { yylval.sval = strdup(yytext); return DTYPE_8; }
"u16"|"s16"                 { yylval.sval = strdup(yytext); return DTYPE_16; }
"u32"|"s32"|"b32"           { yylval.sval = strdup(yytext); return DTYPE_32; }
"u64"|"s64"|"b64"           { yylval.sval = strdup(yytext); return DTYPE_64; }
"f32"                       { yylval.sval = strdup(yytext); return DTYPE_F32; }
"f64"                       { yylval.sval = strdup(yytext); return DTYPE_F64; }

"global"                    { return GLOBAL; }
"shared"                    { return SHARED; }
"local"                     { return LOCAL; }
"const"                     { return CONST; }
"texture"                   { return TEXTURE; }

"lo"|"hi"                   { yylval.sval = strdup(yytext); return MODIFIER; }
"wide"                      { yylval.sval = strdup(yytext); return MODIFIER; }
"rn"|"rz"|"rm"|"rp"         { yylval.sval = strdup(yytext); return MODIFIER; }
"to"                        { yylval.sval = strdup(yytext); return MODIFIER; }
"ge"|"lt"|"le"|"gt"|"eq"|"ne" { yylval.sval = strdup(yytext); return MODIFIER; }

"%tid.x"                    { yylval.sval = strdup("tid.x"); return BUILTIN_VAR; }
"%tid.y"                    { yylval.sval = strdup("tid.y"); return BUILTIN_VAR; }
"%tid.z"                    { yylval.sval = strdup("tid.z"); return BUILTIN_VAR; }
"%ntid.x"                   { yylval.sval = strdup("ntid.x"); return BUILTIN_VAR; }
"%ntid.y"                   { yylval.sval = strdup("ntid.y"); return BUILTIN_VAR; }
"%ntid.z"                   { yylval.sval = strdup("ntid.z"); return BUILTIN_VAR; }
"%ctaid.x"                  { yylval.sval = strdup("ctaid.x"); return BUILTIN_VAR; }
"%ctaid.y"                  { yylval.sval = strdup("ctaid.y"); return BUILTIN_VAR; }
"%ctaid.z"                  { yylval.sval = strdup("ctaid.z"); return BUILTIN_VAR; }

"%"{IDENTIFIER}             { yylval.sval = strdup(yytext + 1); return REGISTER; }

{INTEGER}                   { yylval.ival = atoi(yytext); return INTEGER_LITERAL; }
{FLOAT}                     { yylval.fval = atof(yytext); return FLOAT_LITERAL; }
{STRING}                    { 
                              char* str = strdup(yytext);
                              str[strlen(str)-1] = '\0';
                              yylval.sval = str + 1;
                              return STRING_LITERAL; 
                            }

{IDENTIFIER}                { yylval.sval = strdup(yytext); return IDENTIFIER; }

"$L__"{ALNUM}*":"           { 
                              char* str = strdup(yytext);
                              str[strlen(str)-1] = '\0';
                              yylval.sval = str;
                              return LABEL;
                            }
"$L__"{ALNUM}*              { yylval.sval = strdup(yytext); return LABEL_REF; }

"."                         { return DOT; }
","                         { return COMMA; }
";"                         { return SEMICOLON; }
":"                         { return COLON; }
"("                         { return LPAREN; }
")"                         { return RPAREN; }
"{"                         { return LBRACE; }
"}"                         { return RBRACE; }
"["                         { return LBRACKET; }
"]"                         { return RBRACKET; }
"<"                         { return LT; }
">"                         { return GT; }
"@"                         { return AT; }
"!"                         { return BANG; }

.                           { yyerror("Unknown character"); }

%%
