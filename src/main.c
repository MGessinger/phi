#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>

token *curtok;

Expr* handleDefinition ()
{
	Expr* e = parseDefinition();
	fprintf(stderr, "Parsed a Function Definition\n");
	if (e == NULL)
		gettok();
	return e;
}

Expr* handleExtern ()
{
	Expr* e = parseExtern();
	fprintf(stderr, "Parsed an external Prototype\n");
	if (e == NULL)
		gettok();
	return e;
}

Expr* handleTopLevelExpr ()
{
	Expr* e = parseTopLevelExpr();
	fprintf(stderr, "Parsed a top-level expression.\n");
	if (e == NULL)
		gettok();
	return e;
}

void REPL ()
{
	Expr *tmp = NULL;
	fprintf(stderr, "> ");
	gettok();
	while (1)
	{
		switch (curtok->tok_type)
		{
			case tok_eof:
				tmp = NULL;
				return;
			case ';': /* Skip top-level semicolons */
				tmp = NULL;
				gettok();
				break;
			case tok_def:
				tmp = handleDefinition();
				break;
			case tok_extern:
				tmp = handleExtern();
				break;
			default:
				tmp = handleTopLevelExpr();
				break;
		}
		printf("%p\n", (void*)tmp);
		clearExpr(tmp);
		fprintf(stderr, "> ");
	}
}

int main (int argc, char **argv)
{
	if (argc > 1)
		return -1;
	argv[0] = '\0';
	curtok = makeToken(32);

	REPL();

	clearToken(curtok);
	return 0;
}
