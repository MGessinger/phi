#include <stdlib.h>
#include "ast.h"

Expr* newExpression (int expr_type)
{
	Expr *e = malloc(sizeof(Expr));
	if (e == NULL)
		return logError("Could not allocate Memory", 0x100);
	e->expr_type = expr_type;
	return e;
}

Expr* newNumberExpr (double val)
{
	Expr *e = newExpression(expr_number);
	if (e == NULL)
		return NULL;
	NumExpr *ne = malloc(sizeof(NumExpr));
	if (ne == NULL)
	{
		free(e);
		return logError("Could not allocate Memory.", 0x100);
	}
	ne->val = val;
	e->expr = ne;
	return e;
}

Expr* newBinaryExpr (int binop, Expr *LHS, Expr *RHS)
{
	Expr *e = newExpression(expr_binop);
	if (e == NULL)
		return NULL;
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
	return e;
}

Expr* newIdentExpr (char *name)
{
	Expr *e = newExpression(expr_ident);
	if (e == NULL)
		return NULL;
	IdentExpr *ie = malloc(sizeof(IdentExpr));
	if (ie == NULL)
	{
		free(e);
		return logError("Could not allocate Memory", 0x100 + expr_ident);
	}
	ie->name = name;
	e->expr = ie;
	return e;
}

Expr* newProtoExpr (char *name, stack *in, stack *out)
{
	Expr *e = newExpression(expr_proto);
	if (e == NULL)
		return NULL;
	ProtoExpr *pe = malloc(sizeof(ProtoExpr));
	if (pe == NULL)
	{
		free(e);
		return logError("Could not allocate Memory.", 0x100 + expr_proto);
	}
	pe->name = name;
	pe->inArgs = in;
	pe->outArgs = out;
	e->expr = pe;
	return e;
}

Expr* newFunctionExpr (Expr *proto, Expr *body)
{
	Expr *e = newExpression(expr_func);
	if (e == NULL)
		return NULL;
	FunctionExpr *fe = malloc(sizeof(FunctionExpr));
	if (fe == NULL)
	{
		free(e);
		return logError("Could not allocate Memory.", 0x100 + expr_func);
	}
	fe->proto = proto;
	fe->body = body;
	e->expr = fe;
	return e;
}

void clearBinOpExpr (BinaryExpr *be)
{
	if (be == NULL)
		return;
	clearExpr(be->LHS);
	clearExpr(be->RHS);
}

void clearIdentExpr (IdentExpr *ie)
{
	if (ie == NULL)
		return;
	free(ie->name);
}

void clearProtoExpr (ProtoExpr *pe)
{
	if (pe == NULL)
		return;
	clearStack(pe->inArgs, free);
	clearStack(pe->outArgs, free);
	free(pe->name);
}

void clearFunctionExpr (FunctionExpr *fe)
{
	if (fe == NULL)
		return;
	clearExpr(fe->proto);
	clearExpr(fe->body);
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
		case expr_ident:
			clearIdentExpr(e->expr);
			break;
		case expr_proto:
			clearProtoExpr(e->expr);
			break;
		case expr_func:
			clearFunctionExpr(e->expr);
			break;
		default:
			break;
	}
	free(e->expr);
	free(e);
}
