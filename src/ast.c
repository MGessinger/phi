#include <stdlib.h>
#include "ast.h"

/*-------------*\
 * Create Data *
\*-------------*/

Expr* newExpression (ExprType expr_type)
{
	Expr *e = malloc(sizeof(Expr));
	if (e == NULL)
		return logError("Could not allocate Memory", 0x100);
	e->expr_type = expr_type;
	return e;
}

Expr* newLiteralExpr (double val, int type)
{
	Expr *e = newExpression(expr_literal);
	if (e == NULL)
		return NULL;
	LiteralExpr *ne = malloc(sizeof(LiteralExpr));
	if (ne == NULL)
	{
		free(e);
		return logError("Could not allocate Memory.", 0x101);
	}
	if (type == lit_real)
		ne->val.real = val;
	else
		ne->val.integral = val;
	ne->type = type;
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
		return logError("Could not allocate Memory.", 0x102);
	}
	be->op = binop;
	be->LHS = LHS;
	be->RHS = RHS;
	e->expr = be;
	return e;
}

Expr* newIdentExpr (char *name, IdFlag flag, unsigned size)
{
	Expr *e = newExpression(expr_ident);
	if (e == NULL)
		return NULL;
	IdentExpr *ie = malloc(sizeof(IdentExpr));
	if (ie == NULL)
	{
		free(e);
		return logError("Could not allocate Memory.", 0x103);
	}
	ie->name = name;
	ie->flag = flag;
	ie->size = size;
	e->expr = ie;
	return e;
}

Expr* newAccessExpr (Expr *ie, Expr *idx)
{
	Expr *e = newExpression(expr_access);
	if (e == NULL)
		return NULL;
	AccessExpr *ae = malloc(sizeof(AccessExpr));
	if (ae == NULL)
	{
		free(e);
		return logError("Could not allocate Memory.", 0x103);
	}
	IdentExpr *ide = ie->expr;
	ae->name = ide->name;
	ae->flag = ide->flag;
	ae->idx = idx;
	e->expr = ae;
	free(ie->expr);
	free(ie);
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
		return logError("Could not allocate Memory.", 0x104);
	}
	pe->name = name;
	pe->inArgs = in;
	pe->outArgs = out;
	e->expr = pe;
	return e;
}

Expr* newFunctionExpr (Expr *proto, Expr *body, Expr *ret)
{
	Expr *e = newExpression(expr_func);
	if (e == NULL)
		return NULL;
	FunctionExpr *fe = malloc(sizeof(FunctionExpr));
	if (fe == NULL)
	{
		free(e);
		return logError("Could not allocate Memory.", 0x105);
	}
	fe->proto = proto;
	fe->body = body;
	fe->ret = ret;
	e->expr = fe;
	return e;
}

Expr* newCondExpr (Expr *Cond, Expr *True, Expr *False)
{
	Expr *e = newExpression(expr_conditional);
	if (e == NULL)
		return NULL;
	CondExpr *ce = malloc(sizeof(CondExpr));
	if (ce == NULL)
	{
		free(e);
		return logError("Could not allocate Memory.", 0x107);
	}
	ce->Cond = Cond;
	ce->True = True;
	ce->False = False;
	e->expr = ce;
	return e;
}

Expr* newLoopExpr (Expr *Cond, Expr *Body, Expr *Else)
{
	Expr *e = newExpression(expr_loop);
	if (e == NULL)
		return NULL;
	LoopExpr *le = malloc(sizeof(LoopExpr));
	if (le == NULL)
	{
		free(e);
		return logError("Could not allocate Memory.", 0x107);
	}
	le->Cond = Cond;
	le->Body = Body;
	le->Else = Else;
	e->expr = le;
	return e;
}

/*----------------------*\
 *	Clear Data	*
\*----------------------*/

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

void clearAccessExpr (AccessExpr *ae)
{
	if (ae == NULL)
		return;
	free(ae->name);
	clearExpr(ae->idx);
}
void clearProtoExpr (ProtoExpr *pe)
{
	if (pe == NULL)
		return;
	clearStack(&(pe->inArgs), free);
	clearStack(&(pe->outArgs), free);
	free(pe->name);
}

void clearFunctionExpr (FunctionExpr *fe)
{
	if (fe == NULL)
		return;
	clearExpr(fe->proto);
	clearExpr(fe->body);
	clearExpr(fe->ret);
}

void clearCondExpr (CondExpr *ce)
{
	if (ce == NULL)
		return;
	clearExpr(ce->Cond);
	clearExpr(ce->True);
	clearExpr(ce->False);
}

void clearLoopExpr (LoopExpr *le)
{
	if (le == NULL)
		return;
	clearExpr(le->Cond);
	clearExpr(le->Body);
	clearExpr(le->Else);
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
		case expr_access:
			clearAccessExpr(e->expr);
			break;
		case expr_proto:
			clearProtoExpr(e->expr);
			break;
		case expr_func:
			clearFunctionExpr(e->expr);
			break;
		case expr_conditional:
			clearCondExpr(e->expr);
			break;
		case expr_loop:
			clearLoopExpr(e->expr);
			break;
		default:
			break;
	}
	free(e->expr);
	free(e);
}
