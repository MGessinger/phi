%{
#include "lexer.h"
%}
%x COMMENT

TYPE		Number|String|Bool
IDENT		[a-zA-Z_]*
%%
"/*"			{ BEGIN(COMMENT); }
<COMMENT>"*/"		{ BEGIN(INITIAL); }
<COMMENT>\n		|
<COMMENT>.		|
"//".*			|
[ \t\n]

<<EOF>>			return tok_eof;
new			return tok_def;
extern			return tok_extern;
"->"			return tok_arrow;

([0-9]*".")?[0-9]+	{ curtok->numVal = strtod(yytext, NULL); return tok_number; }
{TYPE}			{ curtok->identStr = yytext; return tok_typename; }
{IDENT}			{ curtok->identStr = yytext; return tok_ident; }
.			return yytext[0];
%%
token* makeToken ()
{
	token *tk = malloc(sizeof(token));
	if (tk == NULL)
		return NULL;
	tk->identStr = NULL;
	tk->numVal = 0;
	tk->tok_type = 0;
	return tk;
}

void clearToken (token *tk)
{
	free(tk);
	yylex_destroy();
}

int yywrap ()
{
	return 1;
}
int gettok()
{
	int tok_type = yylex();
	curtok->tok_type = tok_type;
	return tok_type;
}
