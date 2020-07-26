#include <llvm-c/Types.h>
#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <stdlib.h>

#include "ast.h"
#include "parser.h"
#include "codegen.h"

static LLVMContextRef llvm_context;
static LLVMModuleRef llvm_module;
static LLVMBuilderRef llvm_builder;

void initialiseLLVM ()
{
	LLVMPassRegistryRef passreg = LLVMGetGlobalPassRegistry();
	LLVMInitializeCore(passreg);

	llvm_context = LLVMGetGlobalContext();
	llvm_builder = LLVMCreateBuilderInContext(llvm_context);
	llvm_module = LLVMModuleCreateWithNameInContext("phi_conpiler_module", llvm_context);
}

void shutdownLLVM ()
{
	LLVMDisposeBuilder(llvm_builder);
	LLVMDumpModule(llvm_module);
	LLVMDisposeModule(llvm_module);
	LLVMShutdown();
}

LLVMValueRef codegenNumExpr (NumExpr *ne)
{
	LLVMTypeRef doubleType = LLVMDoubleTypeInContext(llvm_context);
	return LLVMConstReal(doubleType, ne->val);
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
			val = LLVMBuildFAdd(llvm_builder, l, r, "addtmp");
			break;
		case '-':
			val = LLVMBuildFSub(llvm_builder, l, r, "subtmp");
			break;
		case '*':
			val = LLVMBuildFMul(llvm_builder, l, r, "multmp");
			break;
		case '<':
			val = LLVMBuildFCmp(llvm_builder, LLVMRealOLT, l, r, "lttmp");
			break;
		default:
			val = logError("Unrecognized binary operator!", 0x2001);
			break;
	}
	return val;
}

LLVMValueRef codegenProtoExpr (ProtoExpr *pe)
{
	LLVMTypeRef doubleType = LLVMDoubleTypeInContext(llvm_context);
	int numOfInputArgs = (pe->inArgs + pe->outArgs - 1);
	LLVMTypeRef *args = malloc(numOfInputArgs*sizeof(LLVMTypeRef));
	if (args == NULL)
		return NULL;
	for (int i = 0; i < numOfInputArgs; i++)
		args[i] = doubleType;

	LLVMTypeRef funcType = LLVMFunctionType(doubleType, args, numOfInputArgs, 0);
	LLVMValueRef Fn = LLVMAddFunction(llvm_module, pe->name, funcType);
	/* Give names to the arguments of Fn! */
	free(args);
	return Fn;
}

LLVMValueRef codegenFuncExpr (FunctionExpr *fe)
{
	ProtoExpr *pe = fe->proto->expr;
	/* Test if a function has been declared before */
	LLVMValueRef function = LLVMGetNamedFunction(llvm_module, pe->name);
	if (function == NULL)
		function = codegenProtoExpr(pe);
	if (function == NULL)
		return NULL;

	LLVMBasicBlockRef bodyBlock = LLVMAppendBasicBlockInContext(llvm_context, function, "bodyEntry");
	LLVMPositionBuilderAtEnd(llvm_builder, bodyBlock);

	/* Store argument in scope! */
	LLVMValueRef body = codegen(fe->body);
	if (body == NULL)
	{
		LLVMDeleteGlobal(function);
		return NULL;
	}
	LLVMBuildRet(llvm_builder, body);
	LLVMVerifyFunction(function, LLVMPrintMessageAction);
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
		case expr_proto:
			return codegenProtoExpr(e->expr);
		case expr_func:
			return codegenFuncExpr(e->expr);
		default:
			logError("Cannot generate IR for unrecognized expression type!", 0x2000);
	}
	return NULL;
}
