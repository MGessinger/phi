#include "ast.h"
#include "parser.h"
#include <stdlib.h>

Expr* newBinaryExpr (int binop, Expr *LHS, Expr *RHS)
{
	Expr *e = malloc(sizeof(Expr));
	if (e == NULL)
		return logError("Could not allocate Memory", 0x100);
	BinaryExpr *be = malloc(sizeof(BinaryExpr));
	if (be == NULL)
	{
		free(e);
		return logError("Could not allocate Memory.", 0x100 + expr_binop);
	}
	be->op = binop;
	be->LHS = LHS;
	be->RHS = RHS;
	e->expr = be;
	e->expr_type = expr_binop;
	return e;
}

Expr* newNumberExpr (double val)
{
	Expr *e = malloc(sizeof(Expr));
	if (e == NULL)
		return logError("Could not allocate Memory", 0x100);
	NumExpr *ne = malloc(sizeof(NumExpr));
	if (ne == NULL)
	{
		free(e);
		return logError("Could not allocate Memory.", 0x100);
	}
	ne->val = val;
	e->expr_type = expr_number;
	e->expr = ne;
	return e;
}

Expr* newProtoExpr (char *name, int in, int out)
{
	Expr *e = malloc(sizeof(Expr));
	if (e == NULL)
		return logError("Could not allocate Memory", 0x100);
	ProtoExpr *pe = malloc(sizeof(ProtoExpr));
	if (pe == NULL)
	{
		free(e);
		return logError("Could not allocate Memory.", 0x100 + expr_proto);
	}
	pe->name = name;
	pe->inArgs = in;
	pe->outArgs = out;
	return e;
}

void clearBinOpExpr (BinaryExpr *be)
{
	if (be == NULL)
		return;
	clearExpr(be->LHS);
	clearExpr(be->RHS);
}

void clearExpr (Expr *e)
{
	if (e == NULL)
		return;
	switch (e->expr_type)
	{
		case expr_binop:
			clearBinOpExpr(e->expr);
			break;
		default:
			break;
	}
	free(e->expr);
	free(e);
}
