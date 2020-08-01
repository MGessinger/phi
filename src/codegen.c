#include <llvm-c/Types.h>
#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>
#include <llvm-c/Transforms/Scalar.h>
#include <llvm-c/Transforms/Utils.h>
#include <stdlib.h>
#include <string.h>

#include "stack.h"
#include "ast.h"
#include "parser.h"
#include "codegen.h"

static LLVMContextRef phi_context;
static LLVMModuleRef phi_module;
static LLVMBuilderRef phi_builder, alloca_builder;
static LLVMPassManagerRef phi_passManager;

static stack *namesInScope;

LLVMPassManagerRef setupPassManager (LLVMModuleRef m)
{
	LLVMPassManagerRef pmr = LLVMCreateFunctionPassManagerForModule(m);
	LLVMAddInstructionCombiningPass(pmr);
	LLVMAddReassociatePass(pmr);
	LLVMAddGVNPass(pmr);
	LLVMAddCFGSimplificationPass(pmr);
	LLVMAddConstantPropagationPass(pmr);
	LLVMAddPromoteMemoryToRegisterPass(pmr);
	return pmr;
}

LLVMBool setupTargetMachine ()
{
	LLVMInitializeAllTargetInfos();
	LLVMInitializeAllTargets();
	LLVMInitializeAllTargetMCs();
	LLVMInitializeAllAsmParsers();
	LLVMInitializeAllAsmPrinters();

	char *triple = LLVMGetDefaultTargetTriple();
	LLVMTargetRef Target;
	char *errorMsg;
	if (LLVMGetTargetFromTriple(triple, &Target, &errorMsg) != 0)
	{
		logError(errorMsg, 0x2700);
		LLVMDisposeMessage(errorMsg);
		return 0;
	}

	LLVMTargetMachineRef TM = LLVMCreateTargetMachine(Target, triple, "generic", "",
								LLVMCodeGenLevelDefault,
								LLVMRelocDefault,
								LLVMCodeModelDefault);
	LLVMTargetDataRef targetData = LLVMCreateTargetDataLayout(TM);
	LLVMSetModuleDataLayout(phi_module, targetData);
	LLVMSetTarget(phi_module, triple);
	LLVMDisposeMessage(triple);
	LLVMDisposeTargetMachine(TM);
	LLVMDisposeTargetData(targetData);
	return 1;
}

void initialiseLLVM ()
{
	LLVMPassRegistryRef passreg = LLVMGetGlobalPassRegistry();
	LLVMInitializeCore(passreg);

	phi_context = LLVMGetGlobalContext();
	phi_builder = LLVMCreateBuilderInContext(phi_context);
	alloca_builder = LLVMCreateBuilderInContext(phi_context);

	phi_module = LLVMModuleCreateWithNameInContext("phi_compiler_module", phi_context);
	phi_passManager = setupPassManager(phi_module);
	setupTargetMachine();

	namesInScope = NULL;
}

void shutdownLLVM ()
{
	clearStack(namesInScope, NULL);
	LLVMDisposeBuilder(phi_builder);
	LLVMDisposeBuilder(alloca_builder);
	LLVMDumpModule(phi_module);
	LLVMFinalizeFunctionPassManager(phi_passManager);
	LLVMDisposePassManager(phi_passManager);
	LLVMDisposeModule(phi_module);
	LLVMShutdown();
}

LLVMValueRef CreateEntryPointAlloca (LLVMValueRef func, const char *name)
{
	LLVMBasicBlockRef entryBlock;
	if (func == NULL)
		entryBlock = LLVMGetInsertBlock(phi_builder);
	else
		entryBlock = LLVMGetEntryBasicBlock(func);
	if (entryBlock == NULL)
		return logError("Attempting to Create Variable in empty function!", 0x2601);
	LLVMPositionBuilderAtEnd(alloca_builder, entryBlock);
	LLVMTypeRef doubleType = LLVMDoubleTypeInContext(phi_context);
	LLVMValueRef alloca = LLVMBuildAlloca(alloca_builder, doubleType, name);
	return alloca;
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

	return logError("Unrecognized identifier!", 0x2101);
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
		case ':':
			val = r;
			break;
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
		case '>':
			val = LLVMBuildFCmp(phi_builder, LLVMRealOGT, l, r, "gttmp");
			break;
		default:
			val = logError("Unrecognized binary operator!", 0x2201);
			break;
	}
	return val;
}

LLVMValueRef codegenProtoExpr (ProtoExpr *pe)
{
	LLVMTypeRef doubleType = LLVMDoubleTypeInContext(phi_context);
	int numOfInputArgs = depth(pe->inArgs);
	LLVMTypeRef args[numOfInputArgs];
	stack *runner = pe->inArgs;
	/* Careful: This loop reverses the order of the arguments! */
	for (int i = 0; i < numOfInputArgs; i++)
	{
		switch (runner->misc)
		{
			case type_number:
				args[i] = doubleType;
				break;
			default:
				args[i] = doubleType;
				break;
		}
		runner = runner->next;
	}

	LLVMTypeRef funcType = LLVMFunctionType(doubleType, args, numOfInputArgs, 0);
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
		return logError("Cannot redefine function. This definition will be ignored.", 0x2401);
	unsigned paramCount = LLVMCountParams(function);
	if (paramCount != depth(pe->inArgs))
		return logError("Mismatch between prototype and definition!", 0x2402);

	/* Build function body recursively */
	LLVMBasicBlockRef bodyBlock = LLVMAppendBasicBlockInContext(phi_context, function, "bodyEntry");
	LLVMPositionBuilderAtEnd(phi_builder, bodyBlock);

	/* Give names to the function arguments. That way we can refer back to them in the function body. */
	LLVMValueRef args[paramCount];
	LLVMGetParams(function, args);
	for (int i = paramCount-1; i >= 0; i--)
	{
		char *name = pop(&(pe->inArgs));
		LLVMValueRef v = args[i];
		LLVMSetValueName2(v, name, strlen(name));
		LLVMValueRef alloca = CreateEntryPointAlloca(function, name);
		LLVMBuildStore(phi_builder, v, alloca);
		namesInScope = push(alloca, 1, namesInScope);
		free(name);
	}

	LLVMValueRef body = codegen(fe->body);
	if (body == NULL)
	{
		LLVMDeleteFunction(function);
		return NULL;
	}
	LLVMBuildRet(phi_builder, body);
	LLVMVerifyFunction(function, LLVMPrintMessageAction);
	LLVMRunFunctionPassManager(phi_passManager, function);

	/* Remove local variables from the scope.
	 * Note: These are precisely the first paramCount items on the stack */
	clearStack(namesInScope, NULL);
	namesInScope = NULL;
	return function;
}

LLVMValueRef codegenCommandExpr (CommandExpr *ce)
{
	Expr *last = ce->es[1];
	if (last->expr_type != expr_ident)
		logError("A Command must end with a function call or an assignment.", 0x2501);
	IdentExpr *ie = last->expr;

	/* First, see if the identifier should be a new Variable. If so, push it on the stack. */
	if (ie->flag == id_new)
	{
		LLVMValueRef alloca = CreateEntryPointAlloca(NULL, ie->name);
		namesInScope = push(alloca, 1, namesInScope);
	}
	LLVMValueRef variable = NULL;
	size_t len = strlen(ie->name);
	stack *running = namesInScope;

	/* Now test if we only want a function, and if not then query the variable stack.
	 * If after this loop, we did not return, but a variable was requested, then abort. */
	if (ie->flag == id_func)
		running = NULL;
	while (running != NULL)
	{
		variable = running->item;
		const char *name = LLVMGetValueName(variable);
		if (strncmp(name, ie->name, len) == 0)
		{
			LLVMValueRef value = codegen(ce->es[0]);
			LLVMBuildStore(phi_builder, value, variable);
			return value;
		}
		running = running->next;
	}
	if (ie->flag == id_var)
		return logError("Found explicit Variable request with unknown identifier name!", 0x2502);

	/* If the previous code did not execute, see if the identifier is a function. If so, call it.
	 * The Code will only ever get to this point, if a function is allowed or demanded. */
	LLVMValueRef func = LLVMGetNamedFunction(phi_module, ie->name);
	if (func == NULL)
		return logError("Unrecognized identifier at end of Command.", 0x2503);
	unsigned expectedArgs = LLVMCountParams(func);
	if (expectedArgs != breadth(ce->es[0]))
		return logError("Incorrect number of arguments given to function!", 0x2503);

	/* Iteratively create the code for each argument */
	Expr *args = ce->es[0];
	LLVMValueRef argValues[expectedArgs];
	for (unsigned i = expectedArgs-1; i >= 1; i--)
	{
		CommandExpr *ce = args->expr;
		argValues[i] = codegen(ce->es[1]);
		args = ce->es[0];
	}
	argValues[0] = codegen(args);

	return LLVMBuildCall(phi_builder, func, argValues, expectedArgs, "calltmp");
}

LLVMValueRef codegenCondExpr (CondExpr *ce)
{
	LLVMValueRef cond = codegen(ce->Cond);
	if (cond == NULL)
		return NULL;

	LLVMTypeRef retType = LLVMDoubleTypeInContext(phi_context);
	LLVMValueRef zero = LLVMConstReal(retType, 0.0);
	LLVMValueRef isTrue = LLVMBuildFCmp(phi_builder, LLVMRealONE, cond, zero, "ifcond");

	/* Obtain the current function being built */
	LLVMBasicBlockRef curBlock = LLVMGetInsertBlock(phi_builder);
	LLVMValueRef fn = LLVMGetBasicBlockParent(curBlock);

	/* Create new Blocks for True, False and the Merge */
	LLVMBasicBlockRef TrueBlock = LLVMAppendBasicBlockInContext(phi_context, fn, "TrueBlock");
	LLVMBasicBlockRef FalseBlock = LLVMCreateBasicBlockInContext(phi_context, "FalseBlock");
	LLVMBasicBlockRef MergeBlock = LLVMCreateBasicBlockInContext(phi_context, "MergeBlock");
	LLVMBuildCondBr(phi_builder, isTrue, TrueBlock, FalseBlock);

	/* Build the TrueBock */
	LLVMPositionBuilderAtEnd(phi_builder, TrueBlock);
	LLVMValueRef trueVal = codegen(ce->True);
	if (trueVal == NULL)
		return NULL;
	LLVMBuildBr(phi_builder, MergeBlock);
	TrueBlock = LLVMGetInsertBlock(phi_builder);

	/* Build the FalseBlock */
	LLVMAppendExistingBasicBlock(fn, FalseBlock);
	LLVMPositionBuilderAtEnd(phi_builder, FalseBlock);
	LLVMValueRef falseVal = codegen(ce->False);
	if (falseVal == NULL)
		return NULL;
	LLVMBuildBr(phi_builder, MergeBlock);
	FalseBlock = LLVMGetInsertBlock(phi_builder);

	/* Reunite the branches */
	LLVMAppendExistingBasicBlock(fn, MergeBlock);
	LLVMPositionBuilderAtEnd(phi_builder, MergeBlock);
	LLVMValueRef phiNode = LLVMBuildPhi(phi_builder, retType, "ifPhi");
	LLVMValueRef incomingVals[2] = {trueVal, falseVal};
	LLVMBasicBlockRef incomingBlocks[2] = {TrueBlock, FalseBlock};
	LLVMAddIncoming(phiNode, incomingVals, incomingBlocks, 2);

	return phiNode;
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
		case expr_comm:
			return codegenCommandExpr(e->expr);
		case expr_proto:
			return codegenProtoExpr(e->expr);
		case expr_func:
			return codegenFuncExpr(e->expr);
		case expr_conditional:
			return codegenCondExpr(e->expr);
		default:
			logError("Cannot generate IR for unrecognized expression type!", 0x2000);
	}
	return NULL;
}
