#include <stdlib.h>
#include "ast.h"

/*-------------*\
 * Create Data *
\*-------------*/

Expr* newExpression (void *expr, ExprType expr_type)
{
	Expr *e = malloc(sizeof(Expr));
	if (e == NULL)
	{
		free(expr);
		return logError("Could not allocate Memory", 0x100);
	}
	e->expr = expr;
	e->expr_type = expr_type;
	return e;
}

Expr* newLiteralExpr (double val, int type)
{
	LiteralExpr *ne = malloc(sizeof(LiteralExpr));
	if (ne == NULL)
		return logError("Could not allocate Memory.", 0x101);
	if (type == lit_real)
		ne->val.real = val;
	else
		ne->val.integral = val;
	ne->type = type;
	return newExpression(ne, expr_literal);
}

Expr* newBinaryExpr (int binop, Expr *LHS, Expr *RHS)
{
	BinaryExpr *be = malloc(sizeof(BinaryExpr));
	if (be == NULL)
		return logError("Could not allocate Memory.", 0x102);
	be->op = binop;
	be->LHS = LHS;
	be->RHS = RHS;
	return newExpression(be, expr_binop);
}

Expr* newIdentExpr (char *name, IdFlag flag, unsigned size)
{
	IdentExpr *ie = malloc(sizeof(IdentExpr));
	if (ie == NULL)
		return logError("Could not allocate Memory.", 0x103);
	ie->name = name;
	ie->flag = flag;
	ie->size = size;
	return newExpression(ie, expr_ident);
}

Expr* newAccessExpr (Expr *ie, Expr *idx)
{
	AccessExpr *ae = malloc(sizeof(AccessExpr));
	if (ae == NULL)
		return logError("Could not allocate Memory.", 0x104);
	IdentExpr *ide = ie->expr;
	ae->name = ide->name;
	ae->flag = ide->flag;
	ae->idx = idx;
	free(ie->expr);
	free(ie);
	return newExpression(ae, expr_access);
}

Expr* newProtoExpr (char *name, stack *in, stack *out, int isTemplate)
{
	ProtoExpr *pe = malloc(sizeof(ProtoExpr));
	if (pe == NULL)
		return logError("Could not allocate Memory.", 0x105);
	pe->name = name;
	pe->inArgs = in;
	pe->outArgs = out;
	pe->isTemplate = isTemplate;
	return newExpression(pe, expr_proto);
}

Expr* newFunctionExpr (Expr *proto, Expr *body, Expr *ret)
{
	FunctionExpr *fe = malloc(sizeof(FunctionExpr));
	if (fe == NULL)
		return logError("Could not allocate Memory.", 0x106);
	fe->proto = proto;
	fe->body = body;
	fe->ret = ret;
	return newExpression(fe, expr_func);
}

Expr* newTemplateExpr (char *name, int type_name)
{
	TemplateExpr *te = malloc(sizeof(TemplateExpr));
	if (te == NULL)
		return logError("Could not allocate Memory.", 0x107);
	te->name = name;
	te->type_name = type_name;
	return newExpression(te, expr_template);
}

Expr* newCondExpr (Expr *Cond, Expr *True, Expr *False)
{
	CondExpr *ce = malloc(sizeof(CondExpr));
	if (ce == NULL)
		return logError("Could not allocate Memory.", 0x108);
	ce->Cond = Cond;
	ce->True = True;
	ce->False = False;
	return newExpression(ce, expr_conditional);
}

Expr* newLoopExpr (Expr *Cond, Expr *Body, Expr *Else)
{
	LoopExpr *le = malloc(sizeof(LoopExpr));
	if (le == NULL)
		return logError("Could not allocate Memory.", 0x109);
	le->Cond = Cond;
	le->Body = Body;
	le->Else = Else;
	return newExpression(le, expr_loop);
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

void clearTemplateExpr (TemplateExpr *te)
{
	if (te == NULL)
		return;
	free(te->name);
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
		case expr_template:
			clearTemplateExpr(e->expr);
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
