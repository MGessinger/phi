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

int getTokPrecedence (token *curtok)
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
	return NULL;
}

void* parsePrimary (token *curtok)
{
	switch (curtok->tok_type)
	{
		case tok_number:
			return parseNumberExpr(curtok);
		case tok_ident:
			return parseIdentExpr(curtok);
		case '(':
			return parseParenExpr(curtok);
		default:
			return logError("No method to parse unrecognized token.", 0x1000);
	}
	return NULL;
}

void* parseBinOpRHS (token *curtok, int minPrec, void* LHS)
{
	int binop, nextPrec;
	void *RHS;
	while (LHS != NULL)
	{
		int tokprec = getTokPrecedence(curtok);
		if (tokprec < minPrec)
			return LHS;

		/* Consume the binary operator and parse the first piece of RHS */
		binop = curtok->tok_type;
		gettok(curtok);
		RHS = parsePrimary(curtok);
		if (RHS == NULL)
			return NULL;

		/* Use look-ahead to test the next part of RHS */
		nextPrec = getTokPrecedence(curtok);
		if (tokprec < nextPrec)
		{
			RHS = parseBinOpRHS(curtok, tokprec+1, RHS);
			if (RHS == NULL)
				return NULL;
		}
		LHS = newBinaryExpr (binop, LHS, RHS);
	}
	return LHS;
}

void* parseExpression (token *curtok)
{
	void *LHS = parsePrimary(curtok);
	if (LHS == NULL)
		return NULL;

	return parseBinOpRHS(curtok, 0, LHS);
}
