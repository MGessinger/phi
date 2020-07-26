#include "ast.h"
#include "parser.h"
#include <stdlib.h>

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

Expr* newIdentExpr (char *name)
{
	Expr *e = malloc(sizeof(Expr));
	if (e == NULL)
		return logError("Could not allocate Memory", 0x100);
	IdentExpr *ie = malloc(sizeof(IdentExpr));
	if (ie == NULL)
	{
		free(e);
		return logError("Could not allocate Memory", 0x100 + expr_ident);
	}
	ie->name = name;
	e->expr_type = expr_ident;
	e->expr = ie;
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
	e->expr = pe;
	e->expr_type = expr_proto;
	return e;
}

Expr* newFunctionExpr (Expr *proto, Expr *body)
{
	Expr *e = malloc(sizeof(Expr));
	if (e == NULL)
		return logError("Could not allocate Memory", 0x100);
	FunctionExpr *fe = malloc(sizeof(FunctionExpr));
	if (fe == NULL)
	{
		free(e);
		return logError("Could not allocate Memory.", 0x100 + expr_func);
	}
	fe->proto = proto;
	fe->body = body;
	e->expr = fe;
	e->expr_type = expr_func;
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
	if (pe->name != NULL)
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
