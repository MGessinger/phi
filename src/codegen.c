#include <llvm-c/Types.h>
#include <llvm-c/Core.h>
#include <stdlib.h>

#include "ast.h"
#include "parser.h"
#include "codegen.h"

void initialiseLLVM ()
{
	LLVMPassRegistryRef passreg = LLVMGetGlobalPassRegistry();
	LLVMInitializeCore(passreg);
}

void shutdownLLVM ()
{
	LLVMShutdown();
}

LLVMValueRef codegenNumExpr (NumExpr *ne, LLVMContextRef ctx)
{
	LLVMTypeRef doubleType = LLVMDoubleTypeInContext(ctx);
	return LLVMConstReal(doubleType, ne->val);
}

LLVMValueRef codegenBinaryExpr (BinaryExpr *be, LLVMContextRef ctx)
{
	LLVMBuilderRef builder = LLVMCreateBuilderInContext(ctx);

	LLVMValueRef l = codegen(be->LHS);
	LLVMValueRef r = codegen(be->RHS);
	if (l == NULL || r == NULL)
		return NULL;

	LLVMValueRef val = NULL;
	switch (be->op)
	{
		case '+':
			val = LLVMBuildAdd (builder, l, r, "addtmp");
			break;
		default:
			val = logError("Unrecognized binary operator!", 0x2001);
			break;
	}
	LLVMDisposeBuilder(builder);
	return val;
}

LLVMValueRef codegen (Expr *e)
{
	LLVMContextRef llvmctx = LLVMGetGlobalContext();
	if (e == NULL)
		return NULL;
	switch (e->expr_type)
	{
		case expr_number:
			return codegenNumExpr(e->expr, llvmctx);
		case expr_binop:
			return codegenBinaryExpr(e->expr, llvmctx);
		default:
			logError("Cannot generate IR for unrecognized expression type!", 0x2000);
	}
	return NULL;
}
