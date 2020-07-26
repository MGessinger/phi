#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "codegen.h"

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
	while (curtok->tok_type != tok_eof)
	{
		switch (curtok->tok_type)
		{
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
		clearExpr(tmp);
		fprintf(stderr, "> ");
	}
	fprintf(stderr, "\n");
}

int main (int argc, char **argv)
{
	if (argc > 1)
		return -1;
	argv[0] = '\0';
	curtok = makeToken(32);
	initialiseLLVM();

	REPL();

	clearToken(curtok);
	shutdownLLVM();
	return 0;
}
