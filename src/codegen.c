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
	LLVMTypeRef type = NULL;
	switch (typename & 0xFFF)
	{
		case type_real:
			type = LLVMDoubleTypeInContext(phi_context);
			break;
		case type_bool:
			type = LLVMInt1TypeInContext(phi_context);
			break;
		case type_int:
			type = LLVMInt32TypeInContext(phi_context);
			break;
		default:
			return logError("Unknown Type Name!", 0x2101);
	}
	unsigned arraysize = (typename >> 16);
	unsigned vecsize = (typename >> 12) & 0xF;
	if (vecsize != 0 && arraysize != 0)
		return logError("Vector size exceeds internal limit of 15.", 0x2102);
	else if (vecsize != 0)
		type = LLVMVectorType(type, vecsize);
	else if (arraysize != 0)
		type = LLVMArrayType(type, arraysize);
	return type;
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
	LLVMValueRef val = NULL;
	LLVMTypeRef type;
	switch (le->type)
	{
		case lit_real:
			type = LLVMDoubleTypeInContext(phi_context);
			val = LLVMConstReal(type, le->val.real);
			break;
		case lit_int:
			type = LLVMInt32TypeInContext(phi_context);
			val = LLVMConstInt(type, le->val.integral, 1);
			break;
		case lit_bool:
			type = LLVMInt1TypeInContext(phi_context);
			val = LLVMConstInt(type, (le->val.integral)!=0, 0);
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
	for (int i = numOfReturnTypes-1; i >= 0; i--)
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
	else if (strncmp(ie->name, "dup", 4) == 0)
	{
		if (valueStack == NULL)
			return logError("No value found to be duplicated.", 0x2402);
		int misc = valueStack->misc;
		void *item = valueStack->item;
		valueStack = push(item, misc, valueStack);
		return item;
	}
	/* Now see if the identifier should be a new Variable. If so, create it and push it on the stack. */
	if (ie->flag == id_new)
	{
		LLVMValueRef topOfStack = pop(&valueStack);
		if (topOfStack == NULL)
			return logError("Cannot assign variable without value.", 0x2403);
		LLVMValueRef alloca = CreateEntryPointAlloca(NULL, LLVMTypeOf(topOfStack), ie->name);
		LLVMBuildStore(phi_builder, topOfStack, alloca);
		namesInScope = push(alloca, scope, namesInScope);
		return topOfStack;
	}
	else if (ie->flag == id_vec)
	{
		LLVMValueRef topOfStack = pop(&valueStack);
		if (topOfStack == NULL)
			return logError("Cannot infer Vector Type without value.", 0x2404);
		LLVMTypeRef vectorType = LLVMVectorType(LLVMTypeOf(topOfStack), ie->size);
		LLVMValueRef vecAlloca = CreateEntryPointAlloca(NULL, vectorType, ie->name);
		namesInScope = push(vecAlloca, scope, namesInScope);
		return vecAlloca;
	}
	else if (ie->flag == id_array)
	{
		LLVMValueRef topOfStack = pop(&valueStack);
		if (topOfStack == NULL)
			return logError("Cannot infer Array Type without value.", 0x2404);
		LLVMTypeRef arrayType = LLVMArrayType(LLVMTypeOf(topOfStack), ie->size);
		LLVMValueRef arrAlloca = CreateEntryPointAlloca(NULL, arrayType, ie->name);
		namesInScope = push(arrAlloca, scope, namesInScope);
		return arrAlloca;
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
					LLVMValueRef topOfStack = pop(&valueStack);
					if (LLVMTypeOf(topOfStack) != LLVMGetAllocatedType(variableAlloca))
						return logError("Type mismatch in Variable assignment.", 0x2405);
					LLVMBuildStore(phi_builder, topOfStack, variableAlloca);
					return topOfStack;
				}
			}
		}
		if (ie->flag == id_var)
			return logError("Found explicit Variable request with unknown identifier name!", 0x2406);
	}
	LLVMValueRef tryFunction = codegenCallExpr(ie);
	if (tryFunction != NULL)
		return tryFunction;
	return logError("Unrecognized identifier!", 0x2407);
}

LLVMValueRef codegenAccessExpr (AccessExpr *ae)
{
	if (strncmp(ae->name, "store", 6) == 0)
		return logError("Attempting to access a keyword as a vector.", 0x2501);
	if (strncmp(ae->name, "dup", 4) == 0)
		return logError("Attempting to access a keyword as a vector.", 0x2502);

	stack *globalValueStack = valueStack;
	valueStack = NULL;
	LLVMValueRef idxVal = codegen(ae->idx, 1);
	clearStack(&valueStack, NULL);
	valueStack = globalValueStack;
	if (idxVal == NULL)
		return NULL;

	LLVMTypeRef idxType = LLVMTypeOf(idxVal);
	LLVMTypeKind idxKind = LLVMGetTypeKind(idxType);
	if (idxKind != LLVMIntegerTypeKind)
		return logError("Incompatible Type found in vector index.", 0x2504);

	LLVMValueRef varAlloca = NULL;
	size_t len = strlen(ae->name);
	for (stack *r = namesInScope; r != NULL; r = r->next)
	{
		varAlloca = r->item;
		const char *name = LLVMGetValueName(varAlloca);
		if (strncmp(name, ae->name, len) != 0)
			continue;

		LLVMTypeRef vartype = LLVMGetAllocatedType(varAlloca);
		LLVMTypeKind varkind = LLVMGetTypeKind(vartype);
		if (varkind != LLVMVectorTypeKind && varkind != LLVMArrayTypeKind)
			return logError("Cannot access variable that is not a vector or array.", 0x2505);
		else if (LLVMIsConstant(idxVal))
		{
			int index = LLVMConstIntGetSExtValue(idxVal);
			if (index < 0)
				return logError("Negative index in Array access.", 0x2506);
			if (varkind == LLVMArrayTypeKind)
			{
				int arrayLength = LLVMGetArrayLength(vartype);
				if (index >= arrayLength)
					return logError("Constant index beyond bounds of the array.", 0x2507);
			}
			else if (varkind == LLVMVectorTypeKind)
			{
				int vectorlength = LLVMGetVectorSize(vartype);
				if (index >= vectorlength)
					return logError("Constant index beyond bounds of the vector.", 0x2508);
			}
		}

		LLVMValueRef zero = LLVMConstNull(idxType);
		LLVMValueRef idxs[2] = {zero, idxVal};
		if (ae->flag == id_var || valueStack == NULL || valueStack->misc == 0)
		{
			LLVMValueRef ptr = LLVMBuildGEP(phi_builder, varAlloca, idxs, 2, "geptmp");
			LLVMValueRef load = LLVMBuildLoad(phi_builder, ptr, name);
			valueStack = push(load, 0, valueStack);
			return load;
		}
		else
		{
			LLVMValueRef value = pop(&valueStack);
			LLVMValueRef ptr = LLVMBuildGEP(phi_builder, varAlloca, idxs, 2, "geptmp");
			LLVMBuildStore(phi_builder, value, ptr);
			return value;
		}
	}
	return logError("Attempting to access an unknown vector!", 0x2509);
}

LLVMValueRef codegenBinaryExpr (BinaryExpr *be)
{
	stack *globalValueStack = valueStack;
	valueStack = NULL;

	LLVMValueRef l = codegen(be->LHS, 0);
	clearStack(&valueStack, NULL);
	if (l == NULL)
		return NULL;

	LLVMValueRef r = codegen(be->RHS, 0);
	if (be->op == ':')
	{
		clearStack(&globalValueStack, NULL);
		return r;
	}
	else if (r == NULL)
		return NULL;

	LLVMValueRef val = NULL;
	switch (be->op)
	{
		case '+':
			val = buildAppropriateAddition(l, r);
			break;
		case '-':
			val = buildAppropriateSubtraction(l, r);
			break;
		case '*':
			val = buildAppropriateMultiplication(l, r);
			break;
		case '/':
			val = buildAppropriateDivision(l, r);
			break;
		case '<':
			val = buildAppropriateComparison(l, r);
			break;
		case '=':
			val = buildAppropriateEquality(l, r);
			break;
		case '%':
			val = buildAppropriateModulo(l, r);
			break;
		default:
			return logError("Unrecognized binary operator!", 0x2501);
	}
	clearStack(&valueStack, NULL);
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

	/* Optionally, create variables for all named output parameters */
	do {
		LLVMTypeRef argType = getAppropriateType((pe->outArgs)->misc);
		char *name = pop(&(pe->outArgs));
		if (name != NULL)
		{
			LLVMValueRef alloca = CreateEntryPointAlloca(function, argType, name);
			namesInScope = push(alloca, scope, namesInScope);
			free(name);
		}
	} while (pe->outArgs != NULL);

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
	LLVMBasicBlockRef PreviousBlock = LLVMGetInsertBlock(phi_builder);
	LLVMValueRef fn = LLVMGetBasicBlockParent(PreviousBlock);

	/* Create new Blocks for True, False and the Merge */
	LLVMBasicBlockRef TrueBlock = LLVMAppendBasicBlockInContext(phi_context, fn, "TrueBlock");
	LLVMBasicBlockRef FalseBlock = LLVMCreateBasicBlockInContext(phi_context, "FalseBlock");
	LLVMBasicBlockRef MergeBlock = LLVMCreateBasicBlockInContext(phi_context, "AfterIf");
	LLVMBuildCondBr(phi_builder, cond, TrueBlock, FalseBlock);

	/* Build the TrueBlock */
	clearStack(&valueStack, NULL);
	LLVMPositionBuilderAtEnd(phi_builder, TrueBlock);
	LLVMValueRef trueVal = codegen(ce->True, 1);
	if (trueVal == NULL)
		return NULL;
	LLVMBuildBr(phi_builder, MergeBlock);

	/* Build the FalseBlock */
	clearStack(&valueStack, NULL);
	LLVMAppendExistingBasicBlock(fn, FalseBlock);
	LLVMPositionBuilderAtEnd(phi_builder, FalseBlock);
	if (ce->False != NULL)
	{
		LLVMValueRef falseVal = codegen(ce->False, 1);
		if (falseVal == NULL)
			return NULL;
	}
	LLVMBuildBr(phi_builder, MergeBlock);

	/* Reunite the branches */
	clearStack(&valueStack, NULL);
	LLVMAppendExistingBasicBlock(fn, MergeBlock);
	LLVMPositionBuilderAtEnd(phi_builder, MergeBlock);

	LLVMTypeRef voidType = LLVMVoidTypeInContext(phi_context);
	LLVMValueRef voidVal = LLVMGetUndef(voidType);
	return voidVal;
}

LLVMValueRef codegenLoopExpr (LoopExpr *le)
{
	LLVMValueRef cond = codegen(le->Cond, 0);
	if (cond == NULL)
		return NULL;

	LLVMTypeRef booltype = LLVMInt1TypeInContext(phi_context);
	LLVMTypeRef condtype = LLVMTypeOf(cond);
	LLVMValueRef zero = NULL;
	if (condtype != booltype)
	{
		LLVMTypeKind condkind = LLVMGetTypeKind(condtype);
		zero = LLVMConstNull(condtype);
		if (condkind == LLVMDoubleTypeKind)
			cond = LLVMBuildFCmp(phi_builder, LLVMRealONE, cond, zero, "loopcond");
		else
			return logError("Incompatible Type in conditional expression.", 0x2701);
	}

	/* Obtain the current function being built */
	LLVMBasicBlockRef PreviousBlock = LLVMGetInsertBlock(phi_builder);
	LLVMValueRef fn = LLVMGetBasicBlockParent(PreviousBlock);

	/* Create new Blocks for Body, Else and the Merge */
	LLVMBasicBlockRef BodyBlock = LLVMAppendBasicBlockInContext(phi_context, fn, "LoopBody");
	LLVMBasicBlockRef ElseBlock = LLVMCreateBasicBlockInContext(phi_context, "ElseBlock");
	LLVMBasicBlockRef MergeBlock = LLVMCreateBasicBlockInContext(phi_context, "AfterLoop");
	LLVMBuildCondBr(phi_builder, cond, BodyBlock, ElseBlock);

	/* Build the Loop Body */
	clearStack(&valueStack, NULL);
	LLVMPositionBuilderAtEnd(phi_builder, BodyBlock);
	LLVMValueRef bodyVal = codegen(le->Body, 1);
	if (bodyVal == NULL)
		return NULL;

	/* Rebuild the condition at the end of the Loop Body */
	cond = codegen(le->Cond, 0);
	if (zero != NULL)
		LLVMBuildFCmp(phi_builder, LLVMRealONE, cond, zero, "loopcond");
	LLVMBuildCondBr(phi_builder, cond, BodyBlock, MergeBlock);

	/* Build the Else Block */
	clearStack(&valueStack, NULL);
	LLVMAppendExistingBasicBlock(fn, ElseBlock);
	LLVMPositionBuilderAtEnd(phi_builder, ElseBlock);
	if (le->Else != NULL)
	{
		LLVMValueRef falseVal = codegen(le->Else, 1);
		if (falseVal == NULL)
			return NULL;
	}
	LLVMBuildBr(phi_builder, MergeBlock);

	/* Reunite Branches */
	clearStack(&valueStack, NULL);
	LLVMAppendExistingBasicBlock(fn, MergeBlock);
	LLVMPositionBuilderAtEnd(phi_builder, MergeBlock);

	LLVMTypeRef voidType = LLVMVoidTypeInContext(phi_context);
	LLVMValueRef voidVal = LLVMGetUndef(voidType);
	return voidVal;
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
		case expr_access:
			val = codegenAccessExpr(e->expr);
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
		case expr_loop:
			val = codegenLoopExpr(e->expr);
			break;
		case expr_comm:
		{
			CommandExpr *ce = e->expr;
			if (codegen(ce->head, 0) == NULL)
				return NULL;
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
