#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"
#include "parser.h"

void* logError(const char *errstr, int errcode)
{
	fprintf(stderr, "Error %x: %s\n", errcode, errstr);
	return NULL;
}

void* newBinaryExpr (int binop, void *LHS, void *RHS)
{
	BinaryExpr *be = malloc(sizeof(BinaryExpr));
	if (be == NULL)
		return logError("Could not allocate Memory.", 0x101);
	be->op = binop;
	be->LHS = LHS;
	be->RHS = RHS;
	return be;
}

int getTokPrecedence ()
{
	switch (curtok->tok_type)
	{
		case '<':
			return 10;
		case '+':
		case '-':
			return 20;
		case '*':
			return 40;
		default:
			return -1;
	}
	return -1;
}

void* parseNumberExpr ()
{
	NumExpr *ne = malloc(sizeof(NumExpr));
	if (ne == NULL)
		return logError("Could not allocate Memory.", 0x100);
	ne->val = curtok->numVal;
	gettok();
	return ne;
}

void* parseParenExpr ()
{
	/* Consume the opening parenthesis */
	gettok();
	void *e = parseExpression();
	if (e == NULL)
		return NULL; /* The error was already logged elsewhere! */

	if (curtok->tok_type != ')')
	{
		free(e);
		return logError("expected ')'.", 0x1001);
	}
	gettok();
	return e;
}

void* parseIdentExpr ()
{
	char *identName = curtok->identStr;
	gettok(); // Consume identifier
	return NULL;
}

void* parsePrimary ()
{
	switch (curtok->tok_type)
	{
		case tok_number:
			return parseNumberExpr();
		case tok_ident:
			return parseIdentExpr();
		case '(':
			return parseParenExpr();
		default:
			return logError("No method to parse unrecognized token.", 0x1000);
	}
	return NULL;
}

void* parseBinOpRHS (int minPrec, void* LHS)
{
	int binop, nextPrec;
	void *RHS;
	while (LHS != NULL)
	{
		int tokprec = getTokPrecedence();
		if (tokprec < minPrec)
			return LHS;

		/* Consume the binary operator and parse the first piece of RHS */
		binop = curtok->tok_type;
		gettok();
		RHS = parsePrimary();
		if (RHS == NULL)
			return NULL;

		/* Use look-ahead to test the next part of RHS */
		nextPrec = getTokPrecedence();
		if (tokprec < nextPrec)
		{
			RHS = parseBinOpRHS(tokprec+1, RHS);
			if (RHS == NULL)
				return NULL;
		}
		LHS = newBinaryExpr (binop, LHS, RHS);
	}
	return LHS;
}

void* parseExpression ()
{
	void *LHS = parsePrimary();
	if (LHS == NULL)
		return NULL;

	return parseBinOpRHS(0, LHS);
}
