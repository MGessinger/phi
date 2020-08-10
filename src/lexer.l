%{
#define YY_NO_INPUT
#define YY_NO_UNPUT
#include <string.h>
#include "parser.h"
%}

%x COMMENT

IDENT	[[:alpha:]_][[:alnum:]_]*

HEXI	0[xX][[:xdigit:]]+
DECI	[1-9][[:digit:]]*|0

HEXF	0[Xx][[:xdigit:]]+(.[[:xdigit:]]+)?([pP][+-]?[[:digit:]]+)?|{HEXI}[fF]
DECF	([[:digit:]]*".")?[[:digit:]]+([eE][+-]?[[:digit:]]+)?|{DECI}[fF]

INT	({HEXI}|{DECI})
%%
"/*"			BEGIN(COMMENT);
<COMMENT>"*/"		BEGIN(INITIAL);
<COMMENT>(?s:.)		|
"//".*			|
[[:space:]]

new			return keyword_new;
extern			return keyword_extern;
if			return keyword_if;
else			return keyword_else;
end			return keyword_end;
while			return keyword_while;

Bool			return type_bool;
Real			return type_real;
Int			return type_int;

-?{INT}			{ yylval.integral = strtol(yytext, NULL, 0); return tok_int; }
-?({DECF}|{HEXF})	{ yylval.numerical = atof(yytext); return tok_real; }
True			{ yylval.integral = 1; return tok_bool; }
False			{ yylval.integral = 0; return tok_bool; }

{IDENT}			{ yylval.pointer = strndup(yytext, yyleng); return tok_ident; }
":<"{INT}">"		{ yylval.integral = strtol(yytext+2, NULL, 0); return tok_vec; }
":["{INT}"]"		{ yylval.integral = strtol(yytext+2, NULL, 0); return tok_array; }

":!"			return tok_new;
":v"			return tok_var;
":f"			return tok_func;
"->"			return tok_arrow;

.			return yytext[0];
%%
int yywrap ()
{
	return 1;
}
