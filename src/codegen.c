#include <llvm-c/Types.h>
#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <stdlib.h>
#include <string.h>

#include "stack.h"
#include "ast.h"
#include "parser.h"
#include "codegen.h"
#include "binaryops.h"

LLVMContextRef phi_context;
LLVMModuleRef phi_module;
LLVMBuilderRef phi_builder, alloca_builder;
extern LLVMPassManagerRef phi_passManager;

static stack *namesInScope = NULL;
static int scope = 0;

LLVMTypeRef getAppropriateType (int typename)
{
	switch (typename)
	{
		case type_real:
			return LLVMDoubleTypeInContext(phi_context);
		case type_bool:
			return LLVMInt1TypeInContext(phi_context);
		default:
			return logError("Unknown Type Name!", 0x2301);
	}
	return NULL;
}

LLVMValueRef CreateEntryPointAlloca (LLVMValueRef func, LLVMTypeRef varType, const char *name)
{
	LLVMBasicBlockRef entryBlock;
	if (func == NULL)
		entryBlock = LLVMGetInsertBlock(phi_builder);
	else
		entryBlock = LLVMGetEntryBasicBlock(func);
	if (entryBlock == NULL)
		return logError("Attempting to Create Variable in empty function!", 0x2202);
	LLVMPositionBuilderAtEnd(alloca_builder, entryBlock);
	LLVMValueRef alloca = LLVMBuildAlloca(alloca_builder, varType, name);
	return alloca;
}

LLVMValueRef codegenLiteralExpr (LiteralExpr *le)
{
	LLVMTypeRef doubleType = LLVMDoubleTypeInContext(phi_context);
	LLVMTypeRef boolType = LLVMInt1TypeInContext(phi_context);
	switch (le->type)
	{
		case lit_number:
			return LLVMConstReal(doubleType, le->val);
		case lit_bool:
			return LLVMConstInt(boolType, (le->val)!=0, 0);
		default:
			return logError("Unknown literal type.", 0x200);
	}
	return NULL;
}

LLVMValueRef codegenIdentExpr (IdentExpr *ie)
{
	const char *valName;
	size_t length;
	size_t inplen = strlen(ie->name);

	stack *runner = namesInScope;
	if (ie->flag != id_var && ie->flag != id_any)
		runner = NULL;
	while (runner != NULL)
	{
		LLVMValueRef v = runner->item;
		valName = LLVMGetValueName2(v, &length);
		if (strncmp(valName, ie->name, inplen) == 0)
		{
			LLVMValueRef load = LLVMBuildLoad(phi_builder, v, valName);
			return load;
		}
		runner = runner->next;
	}

	LLVMValueRef func = NULL;
	if (ie->flag == id_func || ie->flag == id_any)
		func = LLVMGetNamedFunction(phi_module, ie->name);
	if (func != NULL)
		return LLVMBuildCall(phi_builder, func, NULL, 0, "calltmp");

	return logError("Unrecognized identifier!", 0x2401);
}


LLVMValueRef codegenBinaryExpr (BinaryExpr *be)
{
	LLVMValueRef l = codegen(be->LHS, 0);
	LLVMValueRef r = codegen(be->RHS, 0);
	if (l == NULL || r == NULL)
		return NULL;

	LLVMTypeRef lhsType = LLVMTypeOf(l);
	LLVMTypeRef rhsType = LLVMTypeOf(r);

	LLVMTypeRef doubletype = LLVMDoubleTypeInContext(phi_context);
	switch (be->op)
	{
		case ':':
			return r;
		case '+':
			return buildAppropriateAddition(l, r);
		case '-':
			if (lhsType != doubletype || rhsType != doubletype)
				return logError("Cannot subtract variables of type other than Real", 0x2502);
			return LLVMBuildFSub(phi_builder, l, r, "subtmp");
		case '*':
			return buildAppropriateMultiplication(l, r);
		case '/':
			if (lhsType != doubletype || rhsType != doubletype)
				return logError("Cannot subtract variables of type other than Real", 0x2502);
			return LLVMBuildFDiv(phi_builder, l, r, "divtmp");
		case '<':
			return buildAppropriateCmp(l, r);
		case '=':
			return buildAppropriateEq(l, r);
		default:
			return logError("Unrecognized binary operator!", 0x2501);
	}
	return NULL;
}

LLVMValueRef codegenProtoExpr (ProtoExpr *pe)
{
	int numOfInputArgs = depth(pe->inArgs);
	LLVMTypeRef args[numOfInputArgs];
	stack *runner = pe->inArgs;
	for (int i = 0; i < numOfInputArgs; i++)
	{
		args[i] = getAppropriateType(runner->misc);
		runner = runner->next;
	}

	LLVMTypeRef ret;
	if (pe->outArgs == NULL)
		ret = LLVMVoidTypeInContext(phi_context);
	else
		ret = getAppropriateType((pe->outArgs)->misc);
	LLVMTypeRef funcType = LLVMFunctionType(ret, args, numOfInputArgs, 0);
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
		return logError("Cannot redefine function. This definition will be ignored.", 0x2601);
	unsigned paramCount = LLVMCountParams(function);
	if (paramCount != depth(pe->inArgs))
		return logError("Mismatch between prototype and definition!", 0x2602);

	/* Build function body recursively */
	LLVMBasicBlockRef bodyBlock = LLVMAppendBasicBlockInContext(phi_context, function, "bodyEntry");
	LLVMPositionBuilderAtEnd(phi_builder, bodyBlock);

	/* Give names to the function arguments. That way we can refer back to them in the function body. */
	LLVMValueRef args[paramCount];
	LLVMGetParams(function, args);
	for (int i = paramCount-1; i >= 0; i--)
	{
		LLVMTypeRef argType = getAppropriateType((pe->inArgs)->misc);
		char *name = pop(&(pe->inArgs));
		LLVMValueRef v = args[i];
		LLVMSetValueName2(v, name, strlen(name));
		LLVMValueRef alloca = CreateEntryPointAlloca(function, argType, name);
		LLVMBuildStore(phi_builder, v, alloca);
		namesInScope = push(alloca, scope, namesInScope);
		free(name);
	}

	LLVMValueRef body = codegen(fe->body, 0);
	if (body == NULL)
	{
		LLVMDeleteFunction(function);
		return NULL;
	}
	LLVMBuildRet(phi_builder, body);
	int verified = LLVMVerifyFunction(function, LLVMPrintMessageAction);
	if (verified == 1)
	{
		LLVMDeleteFunction(function);
		return NULL;
	}
	LLVMRunFunctionPassManager(phi_passManager, function);
	return function;
}

LLVMValueRef codegenCommandExpr (CommandExpr *ce)
{
	Expr *tail = ce->tail;
	if (tail->expr_type != expr_ident)
		return logError("A Command must end with a function call or an assignment.", 0x2701);
	IdentExpr *ie = tail->expr;

	/* First, see if the identifier should be a new Variable. If so, create it and push it on the stack. */
	if (ie->flag == id_new)
	{
		LLVMValueRef incomingValue = codegen(ce->head, 0);
		if (incomingValue == NULL)
			return NULL;
		LLVMValueRef alloca = CreateEntryPointAlloca(NULL, LLVMTypeOf(incomingValue), ie->name);
		LLVMBuildStore(phi_builder, incomingValue, alloca);
		namesInScope = push(alloca, scope, namesInScope);
		return incomingValue;
	}

	/* Now test for an existing variable. */
	if (ie->flag == id_var || ie->flag == id_any)
	{
		LLVMValueRef variable = NULL;
		size_t len = strlen(ie->name);
		stack *running = namesInScope;
		while (running != NULL)
		{
			variable = running->item;
			const char *name = LLVMGetValueName(variable);
			if (strncmp(name, ie->name, len) == 0)
			{
				LLVMValueRef value = codegen(ce->head, 0);
				if (value == NULL)
					return NULL;
				LLVMBuildStore(phi_builder, value, variable);
				return value;
			}
			running = running->next;
		}
		if (ie->flag == id_var)
			return logError("Found explicit Variable request with unknown identifier name!", 0x2702);
	}

	/* If the previous code did not execute, see if the identifier is a function. If so, call it.
	 * The Code will only ever get to this point, if a function is allowed or demanded. */
	LLVMValueRef func = LLVMGetNamedFunction(phi_module, ie->name);
	if (func == NULL)
		return logError("Unrecognized identifier at end of Command.", 0x2703);
	unsigned expectedArgs = LLVMCountParams(func);
	if (expectedArgs != breadth(ce->head))
		return logError("Incorrect number of arguments given to function!", 0x2704);

	/* Iteratively create the code for each argument */
	Expr *args = ce->head;
	LLVMValueRef argValues[expectedArgs];
	for (unsigned i = expectedArgs-1; i >= 1; i--)
	{
		CommandExpr *ce = args->expr;
		argValues[i] = codegen(ce->tail, 0);
		if (argValues[i] == NULL)
			return NULL;
		args = ce->head;
	}
	argValues[0] = codegen(args, 0);
	if (argValues == NULL)
		return NULL;

	return LLVMBuildCall(phi_builder, func, argValues, expectedArgs, "calltmp");
}

LLVMValueRef codegenCondExpr (CondExpr *ce)
{
	LLVMValueRef cond = codegen(ce->Cond, 0);
	if (cond == NULL)
		return NULL;

	LLVMTypeRef booltype = LLVMInt1TypeInContext(phi_context);
	LLVMTypeRef condtype = LLVMTypeOf(cond);
	if (condtype != booltype)
	{
		LLVMTypeKind condkind = LLVMGetTypeKind(condtype);
		LLVMValueRef zero = LLVMConstNull(condtype);
		if (condkind == LLVMDoubleTypeKind)
			cond = LLVMBuildFCmp(phi_builder, LLVMRealONE, cond, zero, "ifcond");
		else
			return logError("Incompatible Type in conditional expression.", 0x2801);
	}

	/* Obtain the current function being built */
	LLVMBasicBlockRef curBlock = LLVMGetInsertBlock(phi_builder);
	LLVMValueRef fn = LLVMGetBasicBlockParent(curBlock);

	/* Create new Blocks for True, False and the Merge */
	LLVMBasicBlockRef TrueBlock = LLVMAppendBasicBlockInContext(phi_context, fn, "TrueBlock");
	LLVMBasicBlockRef FalseBlock = LLVMCreateBasicBlockInContext(phi_context, "FalseBlock");
	LLVMBasicBlockRef MergeBlock = LLVMCreateBasicBlockInContext(phi_context, "MergeBlock");
	LLVMBuildCondBr(phi_builder, cond, TrueBlock, FalseBlock);

	/* Build the TrueBock */
	LLVMPositionBuilderAtEnd(phi_builder, TrueBlock);
	LLVMValueRef trueVal = codegen(ce->True, 1);
	if (trueVal == NULL)
		return NULL;
	LLVMBuildBr(phi_builder, MergeBlock);
	LLVMTypeRef truetype = LLVMTypeOf(trueVal);
	TrueBlock = LLVMGetInsertBlock(phi_builder);

	/* Build the FalseBlock */
	LLVMAppendExistingBasicBlock(fn, FalseBlock);
	LLVMPositionBuilderAtEnd(phi_builder, FalseBlock);
	LLVMValueRef falseVal = codegen(ce->False, 1);
	if (falseVal == NULL)
		return NULL;
	LLVMBuildBr(phi_builder, MergeBlock);
	LLVMTypeRef falsetype = LLVMTypeOf(falseVal);
	FalseBlock = LLVMGetInsertBlock(phi_builder);

	if (truetype != falsetype)
		return logError("Incompatible types found in Conditional Blocks.", 0x2802);

	/* Reunite the branches */
	LLVMAppendExistingBasicBlock(fn, MergeBlock);
	LLVMPositionBuilderAtEnd(phi_builder, MergeBlock);
	LLVMValueRef phiNode = LLVMBuildPhi(phi_builder, truetype, "ifPhi");
	LLVMValueRef incomingVals[2] = {trueVal, falseVal};
	LLVMBasicBlockRef incomingBlocks[2] = {TrueBlock, FalseBlock};
	LLVMAddIncoming(phiNode, incomingVals, incomingBlocks, 2);

	return phiNode;
}

LLVMValueRef codegen (Expr *e, int newScope)
{
	if (e == NULL)
		return NULL;
	scope += (newScope != 0);
	LLVMValueRef val = NULL;
	switch (e->expr_type)
	{
		case expr_literal:
			val = codegenLiteralExpr(e->expr);
			break;
		case expr_binop:
			val = codegenBinaryExpr(e->expr);
			break;
		case expr_ident:
			val = codegenIdentExpr(e->expr);
			break;
		case expr_comm:
			val = codegenCommandExpr(e->expr);
			break;
		case expr_proto:
			val = codegenProtoExpr(e->expr);
			break;
		case expr_func:
			val = codegenFuncExpr(e->expr);
			break;
		case expr_conditional:
			val = codegenCondExpr(e->expr);
			break;
		default:
			val = logError("Cannot generate IR for unrecognized expression type!", 0x2101);
			break;
	}
	scope -= (newScope != 0);
	while (namesInScope != NULL && namesInScope->misc > scope)
		pop(&namesInScope);
	return val;
}
