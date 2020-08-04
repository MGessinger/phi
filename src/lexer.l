%{
#define YY_NO_INPUT
#define YY_NO_UNPUT
#include "parser.h"
char *copyString (const char *str, unsigned cutoff);
%}
%x COMMENT
IDENT [a-zA-Z_][a-zA-Z0-9_]*
%%
"/*"			{ BEGIN(COMMENT); }
<COMMENT>"*/"		{ BEGIN(INITIAL); }
<COMMENT>\n		|
<COMMENT>.		|
"//".*			|
[ \t\n]

new			return keyword_new;
extern			return keyword_extern;
just			return keyword_just;
if			return keyword_if;
else			return keyword_else;
end			return keyword_end;
while			return keyword_while;

Bool			return type_bool;
Real			return type_real;
String			return type_string;

-?([0-9]*".")?[0-9]+	{ yylval.numerical = strtod(yytext, NULL); return tok_number; }
True			{ yylval.numerical = 1; return tok_bool; }
False			{ yylval.numerical = 0; return tok_bool; }

{IDENT}":!"		{ yylval.string = copyString(yytext, 2); return tok_new; }
{IDENT}":v"		{ yylval.string = copyString(yytext, 2); return tok_var; }
{IDENT}":f"		{ yylval.string = copyString(yytext, 2); return tok_func; }
{IDENT}			{ yylval.string = copyString(yytext, 0); return tok_ident; }

"->"			return tok_arr;
.			return yytext[0];
%%
char* copyString (const char *str, unsigned cutoff)
{
	if (str == NULL)
		return NULL;
	size_t len = strlen(str) - cutoff;
	char *copy = malloc((strlen(str)+1)*sizeof(char));
	if (copy == NULL)
		return NULL;
	strcpy(copy, str);
	copy[len] = '\0';
	return copy;
}

int yywrap ()
{
	return 1;
}
