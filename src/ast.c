#include <stdlib.h>
#include "ast.h"

unsigned bredth (Expr *e)
{
	unsigned br = 0;
	while (e->expr_type == expr_comm)
	{
		br++;
		CommandExpr *ce = e->expr;
		e = ce->es[1];
	}
	return br+1;
}

Expr **flatten (Expr *args, unsigned bredth)
{
	Expr **flat = malloc(bredth * sizeof(Expr*));
	if (flat == NULL)
		return logError("Could not allocate Memory.", 0x108);
	for (unsigned i = 0; i >= 1; i--)
	{
		CommandExpr *ce = args->expr;
		free(args);

		flat[i] = ce->es[2];
		args = ce->es[1];
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

Expr* newCallExpr (Expr *funcRef, Expr *args)
{
	if (args == NULL)
		return NULL;
	Expr *e = newExpression(expr_call);
	if (e == NULL)
		return NULL;
	CallExpr *ce = malloc(sizeof(CallExpr));
	if (ce == NULL)
	{
		free(e);
		return logError("Could not allocate Memory.", 0x107);
	}
	NamedExpr *ne = funcRef->expr;
	ce->funcRef = ne->funcRef;
	clearExpr(funcRef);

	ce->numArgs = bredth(args);
	ce->args = flatten(args, ce->numArgs);
	if (ce->args == NULL)
	{
		free(e);
		free(ce);
		return NULL;
	}
	e->expr = ce;
	return e;
}

Expr* newNamedExpr (void *funcRef)
{
	Expr *e = newExpression(expr_named);
	if (e == NULL)
		return NULL;
	NamedExpr *ne = malloc(sizeof(NamedExpr));
	if (ne == NULL)
	{
		free(e);
		return logError("Could not allocate Memory.", 0x109);
	}
	ne->funcRef = funcRef;
	e->expr = ne;
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

void clearCallExpr (CallExpr *ce)
{
	if (ce == NULL)
		return;
	if (ce->args == NULL)
		return;
	for (unsigned i = 0; i < ce->numArgs; i++)
		clearExpr(ce->args[i]);
	free(ce->args);
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
		case expr_call:
			clearCallExpr(e->expr);
			break;
		default:
			break;
	}
	free(e->expr);
	free(e);
}
