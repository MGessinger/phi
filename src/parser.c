#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"
#include "parser.h"

void* logError(const char *errstr, int errcode)
{
	fprintf(stderr, "Error %x: %s\n", errcode, errstr);
	return NULL;
}

void* parseNumberExpr (token *curtok)
{
	NumExpr *ne = malloc(sizeof(NumExpr));
	if (ne == NULL)
		return logError("Could not allocate Memory.", 0x100);
	ne->val = curtok->numVal;
	gettok(curtok);
	return ne;
}

void* parseParenExpr (token *curtok)
{
	/* Consume the opening parenthesis */
	gettok(curtok);
	void *e = parseExpression(curtok);
	if (e == NULL)
		return NULL; /* The error was already logged elsewhere! */

	if (curtok->tok_type != ')')
	{
		free(e);
		return logError("expected ')'.", 0x1001);
	}
	gettok(curtok);
	return e;
}

void* parseIdentExpr (token *curtok)
{
	char *identName = curtok->identStr;
	gettok(curtok); // Consume identifier
}

void* parseExpression (token *curtok)
{
	// TODO: Later
	gettok(curtok);
}

void* parsePrimary (token *curtok)
{
	switch (curtok->tok_type)
	{
		case tok_number:
			return parseNumberExpr(curtok);
		case '(':
			return parseParenExpr(curtok);
		default:
			return logError("Unknown token when expecting an expression", 0x1000);
	}
	return NULL;
}
