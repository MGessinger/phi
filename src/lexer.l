%{
#define YY_NO_INPUT
#define YY_NO_UNPUT
#include "parser.h"
char *copyString (const char *str);
%}
%x COMMENT
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

Bool			return type_bool;
Number			return type_number;
String			return type_string;

([0-9]*".")?[0-9]+	{ yylval.numerical = strtod(yytext, NULL); return tok_number; }
[a-zA-Z_]+		{ yylval.string = copyString(yytext); return tok_ident; }
"->"			return tok_arr;

.			return yytext[0];
%%
char* copyString (const char *str)
{
	if (str == NULL)
		return NULL;
	char *copy = malloc((strlen(str)+1)*sizeof(char));
	if (copy == NULL)
		return NULL;
	strcpy(copy, str);
	return copy;
}

int yywrap ()
{
	return 1;
}
