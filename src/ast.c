#include <stdlib.h>
#include "ast.h"

unsigned breadth (Expr *e)
{
	if (e == NULL)
		return 0;
	unsigned br = 0;
	while (e->expr_type == expr_comm)
	{
		br++;
		CommandExpr *ce = e->expr;
		e = ce->es[0];
	}
	return br+1;
}

Expr **flatten (Expr *args, unsigned breadth)
{
	if (breadth == 0)
		return NULL;
	Expr **flat = malloc(breadth * sizeof(Expr*));
	if (flat == NULL)
		return logError("Could not allocate Memory.", 0x108);
	for (unsigned i = breadth-1; i >= 1; i--)
	{
		CommandExpr *ce = args->expr;
		free(args);

		flat[i] = ce->es[1];
		args = ce->es[0];
		free(ce);
	}
	flat[0] = args;
	return flat;
}

/*-------------*\
 * Create Data *
\*-------------*/

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
		return logError("Could not allocate Memory.", 0x101);
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
		return logError("Could not allocate Memory.", 0x102);
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
		return logError("Could not allocate Memory.", 0x103);
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
		return logError("Could not allocate Memory.", 0x104);
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
		return logError("Could not allocate Memory.", 0x105);
	}
	fe->proto = proto;
	fe->body = body;
	e->expr = fe;
	return e;
}

Expr* newCommandExpr (Expr *e1, Expr *e2)
{
	Expr *e = newExpression(expr_comm);
	if (e == NULL)
		return NULL;
	CommandExpr *ce = malloc(sizeof(CommandExpr));
	if (ce == NULL)
	{
		free(e);
		return logError("Could not allocate Memory.", 0x106);
	}
	ce->es[0] = e1;
	ce->es[1] = e2;
	e->expr = ce;
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

void clearCommandExpr (CommandExpr *ce)
{
	if (ce == NULL)
		return;
	clearExpr(ce->es[0]);
	clearExpr(ce->es[1]);
}

void clearCondExpr (CondExpr *ce)
{
	if (ce == NULL)
		return;
	clearExpr(ce->Cond);
	clearExpr(ce->True);
	clearExpr(ce->False);
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
		case expr_comm:
			clearCommandExpr(e->expr);
			break;
		case expr_conditional:
			clearCondExpr(e->expr);
			break;
		default:
			break;
	}
	free(e->expr);
	free(e);
}
