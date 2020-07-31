#include <llvm-c/Types.h>
#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <stdlib.h>
#include <string.h>

#include "stack.h"
#include "ast.h"
#include "parser.h"
#include "codegen.h"

static LLVMContextRef phi_context;
static LLVMModuleRef phi_module;
static LLVMBuilderRef phi_builder;

static stack *namesInScope;

void initialiseLLVM ()
{
	LLVMPassRegistryRef passreg = LLVMGetGlobalPassRegistry();
	LLVMInitializeCore(passreg);

	phi_context = LLVMGetGlobalContext();
	phi_builder = LLVMCreateBuilderInContext(phi_context);
	phi_module = LLVMModuleCreateWithNameInContext("phi_compiler_module", phi_context);
	namesInScope = NULL;
}

void shutdownLLVM ()
{
	clearStack(namesInScope, NULL);
	LLVMDisposeBuilder(phi_builder);
	LLVMDumpModule(phi_module);
	LLVMDisposeModule(phi_module);
	LLVMShutdown();
}

LLVMValueRef tryGetNamedFunc (const char *name)
{
	return LLVMGetNamedFunction(phi_module, name);
}

LLVMValueRef codegenNumExpr (NumExpr *ne)
{
	LLVMTypeRef doubleType = LLVMDoubleTypeInContext(phi_context);
	return LLVMConstReal(doubleType, ne->val);
}

LLVMValueRef codegenIdentExpr (IdentExpr *ie)
{
	const char *valName;
	size_t length;

	LLVMValueRef v;
	stack *runner = namesInScope;
	while (runner != NULL)
	{
		v = runner->item;
		valName = LLVMGetValueName2(v, &length);
		if (strncmp(valName, ie->name, length))
			return v;
		runner = runner->next;
	}
	v = LLVMGetNamedFunction(phi_module, ie->name);
	if (v == NULL)
		return logError("Unrecognized identifier!", 0x2004);
	return v;
}

LLVMValueRef codegenBinaryExpr (BinaryExpr *be)
{
	LLVMValueRef l = codegen(be->LHS);
	LLVMValueRef r = codegen(be->RHS);
	if (l == NULL || r == NULL)
		return NULL;

	LLVMValueRef val = NULL;
	switch (be->op)
	{
		case '+':
			val = LLVMBuildFAdd(phi_builder, l, r, "addtmp");
			break;
		case '-':
			val = LLVMBuildFSub(phi_builder, l, r, "subtmp");
			break;
		case '*':
			val = LLVMBuildFMul(phi_builder, l, r, "multmp");
			break;
		case '<':
			val = LLVMBuildFCmp(phi_builder, LLVMRealOLT, l, r, "lttmp");
			break;
		default:
			val = logError("Unrecognized binary operator!", 0x2001);
			break;
	}
	return val;
}

LLVMValueRef codegenProtoExpr (ProtoExpr *pe)
{
	LLVMTypeRef doubleType = LLVMDoubleTypeInContext(phi_context);
	int numOfInputArgs = depth(pe->inArgs);
	LLVMTypeRef *args = malloc(numOfInputArgs*sizeof(LLVMTypeRef));
	if (args == NULL)
		return logError("Could not write Function Args!", 0x2002);
	stack *runner = pe->inArgs;
	/* Careful: This loop reverses the order of the arguments! */
	for (int i = 0; i < numOfInputArgs; i++)
	{
		switch (runner->misc)
		{
			case tok_number:
				args[i] = doubleType;
				break;
			default:
				args[i] = doubleType;
				break;
		}
		runner = runner->next;
	}

	LLVMTypeRef funcType = LLVMFunctionType(doubleType, args, numOfInputArgs, 0);
	free(args);
	return LLVMAddFunction(phi_module, pe->name, funcType);
}

LLVMValueRef codegenFuncExpr (FunctionExpr *fe)
{
	ProtoExpr *pe = fe->proto->expr;
	/* Test if a function has been declared before */
	LLVMValueRef function = LLVMGetNamedFunction(phi_module, pe->name);
	if (function == NULL)
	{
		function = codegenProtoExpr(pe);
		if (function == NULL)
			return NULL;
	}
	else if (LLVMCountBasicBlocks(function) != 0)
		return logError("Cannot redefine function. This definition will be ignored.", 0x2003);

	/* Give names to the function arguments. That way we can refer back to them in the function body. */
	unsigned paramCount = LLVMCountParams(function);
	LLVMValueRef *args = malloc(paramCount * sizeof(LLVMValueRef));
	if (args == NULL)
		return logError("Could not read arguments.", 0x2004);
	LLVMGetParams(function, args);
	for (int i = paramCount-1; i >= 0; i--)
	{
		char *name = pop(&(pe->inArgs));
		LLVMValueRef v = args[i];
		LLVMSetValueName2(v, name, strlen(name));
		namesInScope = push(v, 1, namesInScope);
		free(name);
	}
	free(args);

	/* Build function body recursively */
	LLVMBasicBlockRef bodyBlock = LLVMAppendBasicBlockInContext(phi_context, function, "bodyEntry");
	LLVMPositionBuilderAtEnd(phi_builder, bodyBlock);

	LLVMValueRef body = codegen(fe->body);
	if (body == NULL)
	{
		LLVMDeleteFunction(function);
		return NULL;
	}
	LLVMBuildRet(phi_builder, body);
	LLVMVerifyFunction(function, LLVMPrintMessageAction);

	/* Remove local variables from the scope.
	 * Note: These are precisely the first paramCount items on the stack */
	for (unsigned i = 0; i < paramCount; i++)
		pop(&namesInScope);
	return function;
}

LLVMValueRef codegen (Expr *e)
{
	if (e == NULL)
		return NULL;
	switch (e->expr_type)
	{
		case expr_number:
			return codegenNumExpr(e->expr);
		case expr_binop:
			return codegenBinaryExpr(e->expr);
		case expr_ident:
			return codegenIdentExpr(e->expr);
		case expr_proto:
			return codegenProtoExpr(e->expr);
		case expr_func:
			return codegenFuncExpr(e->expr);
		default:
			logError("Cannot generate IR for unrecognized expression type!", 0x2000);
	}
	return NULL;
}
