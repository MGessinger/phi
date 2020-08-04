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

static stack *valueStack = NULL;
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
			return logError("Unknown Type Name!", 0x2101);
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
		return logError("Attempting to Create Variable in empty function!", 0x2201);
	LLVMPositionBuilderAtEnd(alloca_builder, entryBlock);
	LLVMValueRef alloca = LLVMBuildAlloca(alloca_builder, varType, name);
	return alloca;
}

LLVMValueRef codegenLiteralExpr (LiteralExpr *le)
{
	LLVMTypeRef doubleType = LLVMDoubleTypeInContext(phi_context);
	LLVMTypeRef boolType = LLVMInt1TypeInContext(phi_context);
	LLVMValueRef val = NULL;
	switch (le->type)
	{
		case lit_number:
			val = LLVMConstReal(doubleType, le->val);
			break;
		case lit_bool:
			val = LLVMConstInt(boolType, (le->val)!=0, 0);
			break;
		default:
			return logError("Unknown literal type.", 0x2301);
	}
	valueStack = push(val, 0, valueStack);
	return val;
}

LLVMValueRef codegenCallExpr (IdentExpr *ie)
{
	LLVMValueRef func = LLVMGetNamedFunction(phi_module, ie->name);
	if (func == NULL)
		return NULL;
	unsigned expectedArgs = LLVMCountParams(func);
	if (expectedArgs > depth(valueStack))
		return logError("Insufficient number of arguments given to function!", 0x2404);

	/* Iteratively create the code for each argument */
	LLVMValueRef argValues[expectedArgs];
	for (int i = expectedArgs-1; i >= 0; i--)
	{
		LLVMValueRef val = pop(&valueStack);
		if (val == NULL)
			return NULL;
		argValues[i] = val;
	}

	LLVMValueRef result = LLVMBuildCall(phi_builder, func, argValues, expectedArgs, "calltmp");
	LLVMTypeRef returnType = LLVMTypeOf(result);
	LLVMTypeKind returnKind = LLVMGetTypeKind(returnType);
	/* If we only returned a single type, push and return that. */
	if (returnKind != LLVMStructTypeKind)
	{
		valueStack = push(result, 1, valueStack);
		return result;
	}
	unsigned numOfReturnTypes = LLVMCountStructElementTypes(returnType);
	for (unsigned i = 0; i < numOfReturnTypes; i++)
	{
		LLVMValueRef structElement = LLVMBuildExtractValue(phi_builder, result, i, "structelem");
		valueStack = push(structElement, 1, valueStack);
	}
	return result;
}

LLVMValueRef codegenIdentExpr (IdentExpr *ie)
{
	/* First, test for known keywords */
	if (strncmp(ie->name, "store", 6) == 0)
	{
		if (valueStack == NULL)
			return logError("No values found to be stored in variables.", 0x2401);
		stack *runner = valueStack;
		while (runner != NULL)
		{
			runner->misc = 1;
			runner = runner->next;
		}
		return valueStack->item;
	}
	/* Now see if the identifier should be a new Variable. If so, create it and push it on the stack. */
	if (ie->flag == id_new)
	{
		if (valueStack == NULL)
			return logError("Cannot assign variable without value.", 0x2402);
		LLVMValueRef value = pop(&valueStack);
		LLVMValueRef alloca = CreateEntryPointAlloca(NULL, LLVMTypeOf(value), ie->name);
		LLVMBuildStore(phi_builder, value, alloca);
		namesInScope = push(alloca, scope, namesInScope);
		return value;
	}

	/* Now test for an existing variable. */
	if (ie->flag == id_var || ie->flag == id_any)
	{
		LLVMValueRef variableAlloca = NULL;
		size_t len = strlen(ie->name);
		for (stack *r = namesInScope; r != NULL; r = r->next)
		{
			variableAlloca = r->item;
			const char *name = LLVMGetValueName(variableAlloca);
			if (strncmp(name, ie->name, len) == 0)
			{
				if (ie->flag == id_var || valueStack == NULL || valueStack->misc == 0)
				{
					LLVMValueRef load = LLVMBuildLoad(phi_builder, variableAlloca, name);
					valueStack = push(load, 0, valueStack);
					return load;
				}
				else
				{
					LLVMValueRef value = pop(&valueStack);
					LLVMBuildStore(phi_builder, value, variableAlloca);
					return value;
				}
			}
		}
		if (ie->flag == id_var)
			return logError("Found explicit Variable request with unknown identifier name!", 0x2403);
	}
	LLVMValueRef tryFunction = codegenCallExpr(ie);
	if (tryFunction != NULL)
		return tryFunction;
	return logError("Unrecognized identifier!", 0x2405);
}

LLVMValueRef codegenBinaryExpr (BinaryExpr *be)
{
	stack *globalValueStack = valueStack;
	valueStack = NULL;
	LLVMValueRef l = codegen(be->LHS, 0);
	clearStack(&valueStack, NULL);
	LLVMValueRef r = codegen(be->RHS, 0);
	clearStack(&valueStack, NULL);
	if (l == NULL || r == NULL)
		return NULL;

	LLVMTypeRef lhsType = LLVMTypeOf(l);
	LLVMTypeRef rhsType = LLVMTypeOf(r);

	LLVMTypeRef doubletype = LLVMDoubleTypeInContext(phi_context);
	LLVMValueRef val = NULL;
	switch (be->op)
	{
		case ':':
			val = r;
			break;
		case '+':
			val = buildAppropriateAddition(l, r);
			break;
		case '-':
			if (lhsType != doubletype || rhsType != doubletype)
				val = logError("Cannot subtract variables of type other than Real", 0x2502);
			else
				val = LLVMBuildFSub(phi_builder, l, r, "subtmp");
			break;
		case '*':
			val = buildAppropriateMultiplication(l, r);
			break;
		case '/':
			if (lhsType != doubletype || rhsType != doubletype)
				val = logError("Cannot divide variables of type other than Real", 0x2503);
			else
				val = LLVMBuildFDiv(phi_builder, l, r, "divtmp");
			break;
		case '<':
			val = buildAppropriateCmp(l, r);
			break;
		case '=':
			val = buildAppropriateEq(l, r);
			break;
		case '%':
			val = buildAppropriateMod(l, r);
			break;
		default:
			return logError("Unrecognized binary operator!", 0x2501);
	}
	valueStack = push(val, 0, globalValueStack);
	return val;
}

LLVMValueRef codegenProtoExpr (ProtoExpr *pe)
{
	int numOfInputArgs = depth(pe->inArgs);
	int numOfOutputArgs = depth(pe->outArgs);
	LLVMTypeRef args[numOfInputArgs];
	stack *runner = pe->inArgs;
	for (int i = numOfInputArgs-1; i >= 0; i--)
	{
		args[i] = getAppropriateType(runner->misc);
		runner = runner->next;
	}

	LLVMTypeRef retType;
	if (numOfOutputArgs == 0)
		retType = LLVMVoidTypeInContext(phi_context);
	else if (numOfOutputArgs == 1)
		retType = getAppropriateType((pe->outArgs)->misc);
	else
	{
		LLVMTypeRef rettypes[numOfOutputArgs];
		stack *runner = pe->outArgs;
		for (int i = numOfOutputArgs-1; i >= 0; i--)
		{
			rettypes[i] = getAppropriateType(runner->misc);
			runner = runner->next;
		}
		retType = LLVMStructTypeInContext(phi_context, rettypes, numOfOutputArgs, 0);
	}
	LLVMTypeRef funcType = LLVMFunctionType(retType, args, numOfInputArgs, 0);
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
	LLVMTypeRef funcType = LLVMGetElementType(LLVMTypeOf(function));
	LLVMTypeRef returnType = LLVMGetReturnType(funcType);
	if (LLVMGetTypeKind(returnType) == LLVMStructTypeKind)
	{
		unsigned countOfRetValues = LLVMCountStructElementTypes(returnType);
		if (valueStack == NULL || countOfRetValues < depth(valueStack))
		{
			LLVMDeleteFunction(function);
			return logError("Not enough return values specified.", 0x2603);
		}
		LLVMValueRef retValues[countOfRetValues];
		for (int i = countOfRetValues-1; i >= 0; i--)
		{
			retValues[i] = pop(&valueStack);
			if (retValues[i] == NULL)
			{
				LLVMDeleteFunction(function);
				return NULL;
			}
		}
		LLVMBuildAggregateRet(phi_builder, retValues, countOfRetValues);
	}
	else if (valueStack != NULL)
		LLVMBuildRet(phi_builder, pop(&valueStack));
	else
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
			return logError("Incompatible Type in conditional expression.", 0x2701);
	}

	/* Obtain the current function being built */
	LLVMBasicBlockRef curBlock = LLVMGetInsertBlock(phi_builder);
	LLVMValueRef fn = LLVMGetBasicBlockParent(curBlock);

	/* Create new Blocks for True, False and the Merge */
	LLVMBasicBlockRef TrueBlock = LLVMAppendBasicBlockInContext(phi_context, fn, "TrueBlock");
	LLVMBasicBlockRef FalseBlock = LLVMCreateBasicBlockInContext(phi_context, "FalseBlock");
	LLVMBasicBlockRef MergeBlock = LLVMCreateBasicBlockInContext(phi_context, "MergeBlock");
	LLVMBuildCondBr(phi_builder, cond, TrueBlock, FalseBlock);

	/* Build the TrueBlock */
	clearStack(&valueStack, NULL);
	LLVMPositionBuilderAtEnd(phi_builder, TrueBlock);
	LLVMValueRef trueVal = codegen(ce->True, 1);
	if (trueVal == NULL)
		return NULL;
	LLVMBuildBr(phi_builder, MergeBlock);
	LLVMTypeRef truetype = LLVMTypeOf(trueVal);
	TrueBlock = LLVMGetInsertBlock(phi_builder);

	/* Build the FalseBlock */
	clearStack(&valueStack, NULL);
	LLVMAppendExistingBasicBlock(fn, FalseBlock);
	LLVMPositionBuilderAtEnd(phi_builder, FalseBlock);
	LLVMValueRef falseVal = codegen(ce->False, 1);
	if (falseVal == NULL)
		return NULL;
	LLVMBuildBr(phi_builder, MergeBlock);
	LLVMTypeRef falsetype = LLVMTypeOf(falseVal);
	FalseBlock = LLVMGetInsertBlock(phi_builder);

	if (truetype != falsetype)
		return logError("Incompatible types found in Conditional Blocks.", 0x2702);

	/* Reunite the branches */
	clearStack(&valueStack, NULL);
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
		case expr_proto:
			val = codegenProtoExpr(e->expr);
			break;
		case expr_func:
			val = codegenFuncExpr(e->expr);
			break;
		case expr_conditional:
			val = codegenCondExpr(e->expr);
			break;
		case expr_comm:
		{
			CommandExpr *ce = e->expr;
			codegen(ce->head, 0);
			val = codegen(ce->tail, 0);
			break;
		}
		default:
			val = logError("Cannot generate IR for unrecognized expression type!", 0x2001);
			break;
	}
	scope -= (newScope != 0);
	while (namesInScope != NULL && namesInScope->misc > scope)
		pop(&namesInScope);
	return val;
}
