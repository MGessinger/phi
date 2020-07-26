#include <llvm-c/Types.h>
#include <llvm-c/Core.h>
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
	llvm_module = LLVMModuleCreateWithNameInContext("llvm_module", llvm_context);
}

void shutdownLLVM ()
{
	LLVMDisposeBuilder(llvm_builder);
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
		default:
			logError("Cannot generate IR for unrecognized expression type!", 0x2000);
	}
	return NULL;
}
