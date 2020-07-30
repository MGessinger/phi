#include <stdio.h>
#include <stdlib.h>

#include "parser.h"
#include "ast.h"
#include "codegen.h"

int yylex();
int yylex_destroy();

/*
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
*/

int main (int argc, char **argv)
{
	if (argc > 1)
		return -1;
	argv[0] = '\0';

	initialiseLLVM();
	yyparse();
	yylex_destroy();
	shutdownLLVM();

	return 0;
}
