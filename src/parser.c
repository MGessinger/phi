#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "lexer.h"
#include "parser.h"

void* logError(const char *errstr, int errcode)
{
	fprintf(stderr, "Error %x: %s\n", errcode, errstr);
	return NULL;
}

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

Expr* parseNumberExpr ()
{
	Expr *e = newNumberExpr(curtok->numVal);
	gettok();
	return e;
}

Expr* parseParenExpr ()
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

Expr* parseIdentExpr ()
{
	gettok(); // Consume identifier
	return NULL;
}

Expr* parseProtoType ()
{
	int inArgs = 0;
	int outArgs = 0;
	/* Expected syntax: inType1 ... inTypeN -> funcName -> outType1 ... outTypeN */
	while (curtok->tok_type == tok_typename)
	{
		inArgs++;
		gettok();
	}
	if (inArgs != 0)
	{
		if (curtok->tok_type != tok_arrow)
			return logError("Expected \"->\" in function prototype with input types!", 0x1001);
		else
			gettok(); /* Consume "->" */
	}

	if (curtok->tok_type != tok_ident)
		return logError("Expected Function Name in prototype!", 0x1002);
	char *nameCopy = copyString(curtok->identStr);
	gettok(); /* Consume function Name */

	if (curtok->tok_type != tok_arrow)
		return logError("Function must have at least one return type. Are you missing an \"->\"?", 0x1003);
	gettok(); /* Consume "->" */

	while (curtok->tok_type == tok_typename)
		outArgs++;
	if (outArgs == 0)
		return logError("Function must have at least one return type!", 0x1004);

	return newProtoExpr (nameCopy, inArgs, outArgs);
}

Expr* parsePrimary ()
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

Expr* parseBinOpRHS (int minPrec, Expr* LHS)
{
	int binop, nextPrec;
	Expr *RHS;
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

Expr* parseExpression ()
{
	void *LHS = parsePrimary();
	if (LHS == NULL)
		return NULL;

	return parseBinOpRHS(0, LHS);
}
