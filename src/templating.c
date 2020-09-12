#include <stdlib.h>
#include <string.h>
#include <llvm-c/Core.h>
#include "parser.h"
#include "stack.h"
#include "ast.h"
#include "templating.h"
#include "codegen.h"

extern LLVMModuleRef phi_module;
LLVMTypeRef templateType;
static const char *templateTypeName = "";
static stack *templates = NULL;

const char* getTypeName (int type_name)
{
	switch (type_name)
	{
		case type_int:
			return "Int";
		case type_real:
			return "Real";
		case type_bool:
			return "Bool";
		case type_template:
			return templateTypeName;
		default:
			return logError("Unknown type name.", 0x3003);
	}
	return NULL;
}

char *fullTemplateName (const char *bareName, int type_name)
{
	const char *typename = getTypeName(type_name);
	char *fullName = malloc(strlen(bareName) + strlen(typename) + 1);
	if (fullName == NULL)
		return logError("Could not allocate Memory.", 0x301);
	strcpy(fullName, bareName);
	strcat(fullName, typename);
	return fullName;
}

void clearTemplates ()
{
	while (templates != NULL)
	{
		int type = templates->misc;
		void *it = pop(&templates);
		if (type == expr_func)
			clearFunctionExpr(it);
		else
			clearProtoExpr(it);
		free(it);
	}
}

void defineNewTemplate (FunctionExpr *fe)
{
	stack *runner = templates;
	ProtoExpr *peNew = fe->proto->expr;
	while (runner != NULL)
	{
		if (runner->misc == expr_proto)
		{
			ProtoExpr *peOld = runner->item;
			if (strcmp(peOld->name, peNew->name) == 0)
			{
				clearProtoExpr(peOld);
				free(peOld);
				runner->item = fe;
				runner->misc = expr_func;
				return;
			}
		}
		else
		{
			FunctionExpr *feOld = runner->item;
			ProtoExpr *pe = feOld->proto->expr;
			if (strcmp(pe->name, peNew->name) == 0)
			{
				logError("Cannot redefine Template! This definition will be ignored.", 0x3001);
				clearFunctionExpr(fe);
				free(fe);
				return;
			}
		}
		runner = runner->next;
	}
	templates = push(fe, expr_func, templates);
}

void declareNewTemplate (ProtoExpr *pe)
{
	stack *runner = templates;
	/* First, see if a template with this name was already defined. If so, return silently.
	 * Else, push a new template on the stack. */
	ProtoExpr *peOld = NULL;
	while (runner != NULL)
	{
		if (runner->misc == expr_func)
		{
			FunctionExpr *fe = runner->item;
			peOld = fe->proto->expr;
		}
		else
			peOld = runner->item;
		if (strcmp(peOld->name, pe->name) == 0)
			return;
		runner = runner->next;
	}
	templates = push(pe, expr_proto, templates);
}

LLVMValueRef compileTemplateForType (void *e, ExprType expr_type, int type_name)
{
	LLVMTypeRef previousTemplateType = templateType;
	templateType = getAppropriateType(type_name);
	const char *typename = getTypeName(type_name);
	templateTypeName = typename;

	ProtoExpr *pe;
	if (expr_type == expr_func)
	{
		FunctionExpr *fe = (FunctionExpr*)e;
		pe = fe->proto->expr;
	}
	else
		pe = e;
	pe->isTemplate = 0;
	char *bareName = pe->name;
	pe->name = fullTemplateName(bareName, type_name);

	Expr E;
	E.expr = e;
	E.expr_type = expr_type;
	LLVMValueRef val = codegen(&E, 1);

	free(pe->name);
	pe->name = bareName;
	pe->isTemplate = 1;
	templateType = previousTemplateType;
	templateTypeName = "";
	return val;
}

LLVMValueRef tryGetTemplate (const char *bareName, int type_name)
{
	char *fullName = fullTemplateName(bareName, type_name);
	LLVMValueRef function = LLVMGetNamedFunction(phi_module, fullName);
	free(fullName);
	if (function != NULL)
		return function;

	stack *runner = templates;
	while (runner != NULL)
	{
		int type = runner->misc;
		if (type == expr_proto)
		{
			ProtoExpr *pe = runner->item;
			if (strcmp(pe->name, bareName) == 0)
				return compileTemplateForType(pe, expr_proto, type_name);
		}
		else
		{
			FunctionExpr *fe = runner->item;
			ProtoExpr *pe = fe->proto->expr;
			if (strcmp(pe->name, bareName) == 0)
				return compileTemplateForType(fe, expr_func, type_name);
		}
		runner = runner->next;
	}
	return NULL;
}

LLVMValueRef compileTemplatePredefined (Expr *e, int type_name)
{
	if (e == NULL || e->expr_type != expr_func)
		return NULL;
	FunctionExpr *fe = e->expr;
	ProtoExpr *pe = fe->proto->expr;
	char *bareName = fullTemplateName(pe->name, type_name);
	free(pe->name);
	pe->name = bareName;
	return codegen(e, 1);
}
