#include <llvm-c/Types.h>
#include <llvm-c/Core.h>
#include <llvm-c/TargetMachine.h>
#include <llvm-c/Transforms/Scalar.h>
#include <llvm-c/Transforms/Utils.h>
#include <llvm-c/Analysis.h>

#include "llvmcontrol.h"
#include "ast.h"

extern LLVMContextRef phi_context;
extern LLVMModuleRef phi_module;
extern LLVMBuilderRef phi_builder, alloca_builder;

LLVMPassManagerRef phi_passManager;

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

LLVMBool emitObjectFile (const char *filename)
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
		logError(errorMsg, 0x2001);
		LLVMDisposeMessage(errorMsg);
		return 0;
	}

	LLVMTargetMachineRef phi_targetMachine = LLVMCreateTargetMachine(Target, triple, "generic", "",
			LLVMCodeGenLevelDefault, LLVMRelocDefault, LLVMCodeModelDefault);
	LLVMTargetDataRef targetData = LLVMCreateTargetDataLayout(phi_targetMachine);
	LLVMSetModuleDataLayout(phi_module, targetData);
	LLVMSetTarget(phi_module, triple);

	LLVMBool emitFailed = LLVMTargetMachineEmitToFile(phi_targetMachine, phi_module, filename, LLVMObjectFile, &errorMsg);
	if (emitFailed)
	{
		logError(errorMsg, 0x2F01);
		LLVMDisposeMessage(errorMsg);
	}

	LLVMDisposeMessage(triple);
	LLVMDisposeTargetData(targetData);
	LLVMDisposeTargetMachine(phi_targetMachine);
	return !emitFailed;
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
}

void shutdownLLVM ()
{
	char *msg;
	int verified = LLVMVerifyModule(phi_module, LLVMPrintMessageAction, &msg);
	LLVMDisposeMessage(msg);
	if (verified == 0)
	{
#ifdef NDEBUG
		emitObjectCode("output.o");
#else
		LLVMDumpModule(phi_module);
#endif
	}

	LLVMDisposeBuilder(phi_builder);
	LLVMDisposeBuilder(alloca_builder);
	LLVMFinalizeFunctionPassManager(phi_passManager);
	LLVMDisposePassManager(phi_passManager);
	LLVMDisposeModule(phi_module);
	LLVMShutdown();
}
