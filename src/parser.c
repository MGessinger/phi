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
		return NULL;

	if (curtok->tok_type != ')')
	{
		free(e);
		return logError("expected ')'.", 0x1100);
	}
	gettok();
	return e;
}

Expr* parseIdentExpr ()
{
	char *nameCopy = copyString(curtok->identStr);
	gettok(); // Consume identifier
	return newIdentExpr(nameCopy);
}

Expr* parsePrototype ()
{
	int inArgs = 0;
	int outArgs = 0;
	/* Expected syntax: inType1:inName1 ... inTypeN:inNameN -> funcName -> outType1:outName1 ... outTypeN:outNameN */
	while (curtok->tok_type == tok_typename)
	{
		gettok(); /* Consume the type, expecting ':' */
		if (curtok->tok_type != ':')
			return logError("All input parameters must be named. Are you missing a ':'?", 0x1200);
		gettok(); /* Consume the ':', expecting identifier */
		if (curtok->tok_type != tok_ident)
			return logError("All input parameters must be named in the form \"Type:Name\".", 0x1201);
		gettok(); /* Consume the identifier */
		inArgs++;
	}
	if (inArgs != 0)
	{
		if (curtok->tok_type != tok_arrow)
			return logError("Expected \"->\" in function prototype with input types!", 0x1202);
		else
			gettok(); /* Consume "->" */
	}

	if (curtok->tok_type != tok_ident)
		return logError("Expected Function Name in prototype!", 0x1203);
	char *nameCopy = copyString(curtok->identStr);
	gettok(); /* Consume function Name */

	if (curtok->tok_type != tok_arrow)
		return logError("Function must have at least one return type. Are you missing an \"->\"?", 0x1204);
	gettok(); /* Consume "->" */

	while (curtok->tok_type == tok_typename)
	{
		outArgs++;
		gettok(); /* Consume the type name. Might be followed by ':' */
		if (curtok->tok_type == ':')
		{
			gettok(); /* Consume ':', expecting identifier! */
			if (curtok->tok_type != tok_ident)
				return logError("Stray ':' in program. Expected identifier.", 0x1205);
			gettok(); /* Consume the identifier */
		}
	}
	if (outArgs == 0)
		return logError("Function must have at least one return type!", 0x1206);

	return newProtoExpr(nameCopy, inArgs, outArgs);
}

Expr* parseDefinition ()
{
	gettok(); /* Consume "new" */
	Expr *proto = parsePrototype();
	if (proto == NULL)
		return NULL;

	Expr *e = parseExpression();
	if (e == NULL)
	{
		clearExpr(proto);
		return NULL;
	}
	return newFunctionExpr(proto, e);
}

Expr* parseExtern ()
{
	gettok(); /* Consume "extern" */
	return parsePrototype();
}

Expr* parseTopLevelExpr ()
{
	Expr *e = parseExpression();
	if (e == NULL)
		return NULL;
	/* Create an anonymous prototype with no input and one output */
	Expr *anon = newProtoExpr (NULL, 0, 1);
	if (anon == NULL)
	{
		clearExpr(e);
		return NULL;
	}
	Expr *tle = newFunctionExpr(anon, e);
	if (tle == NULL)
	{
		clearExpr(e);
		clearExpr(anon);
		return NULL;
	}
	return tle;
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
		case ';':
			return NULL;
		case tok_eof:
			return logError("File ended while scanning Expression!", 0x1000);
		default:
			return logError("No method to parse unrecognized token.", 0x1001);
	}
	return NULL;
}

Expr* parseBinOpRHS (int minPrec, Expr* LHS)
{
	int binop, nextPrec;
	Expr *RHS;
	int tokPrec = getTokPrecedence();
	while (tokPrec >= minPrec)
	{
		/* Consume the binary operator and parse the first piece of RHS */
		binop = curtok->tok_type;
		gettok();
		RHS = parsePrimary();
		if (RHS == NULL)
			return NULL;

		/* Use look-ahead to test the next part of RHS */
		nextPrec = getTokPrecedence();
		if (tokPrec < nextPrec)
		{
			RHS = parseBinOpRHS(tokPrec+1, RHS);
			if (RHS == NULL)
				return NULL;
		}
		LHS = newBinaryExpr (binop, LHS, RHS);
		tokPrec = getTokPrecedence();
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
