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
static LLVMTargetMachineRef phi_targetMachine;

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
		logError(errorMsg, 0x2001);
		LLVMDisposeMessage(errorMsg);
		return 0;
	}

	phi_targetMachine = LLVMCreateTargetMachine(Target, triple, "generic", "",
			LLVMCodeGenLevelDefault, LLVMRelocDefault, LLVMCodeModelDefault);
	LLVMTargetDataRef targetData = LLVMCreateTargetDataLayout(phi_targetMachine);
	LLVMSetModuleDataLayout(phi_module, targetData);
	LLVMSetTarget(phi_module, triple);
	LLVMDisposeMessage(triple);
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
}

void shutdownLLVM (int emit)
{
	char *msg;
	int verified = LLVMVerifyModule(phi_module, LLVMPrintMessageAction, &msg);
	LLVMDisposeMessage(msg);
	if (verified == 0)
	{
		LLVMDumpModule(phi_module);
		if (emit)
		{
			char *errorMsg;
			LLVMBool emitSuccess = LLVMTargetMachineEmitToFile(phi_targetMachine, phi_module, "output.o", LLVMObjectFile, &errorMsg);
			if (emitSuccess != 0)
			{
				logError(errorMsg, 0x2F01);
				LLVMDisposeMessage(errorMsg);
			}
		}
	}

	LLVMDisposeBuilder(phi_builder);
	LLVMDisposeBuilder(alloca_builder);
	LLVMFinalizeFunctionPassManager(phi_passManager);
	LLVMDisposeTargetMachine(phi_targetMachine);
	LLVMDisposePassManager(phi_passManager);
	LLVMDisposeModule(phi_module);
	LLVMShutdown();
}

