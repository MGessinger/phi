#include <llvm-c/Types.h>
#include <llvm-c/Core.h>
#include "binaryops.h"
#include "ast.h"

extern LLVMBuilderRef phi_builder;
extern LLVMContextRef phi_context;

LLVMValueRef buildAppropriateAddition (LLVMValueRef lhs, LLVMValueRef rhs)
{
	LLVMTypeRef booltype = LLVMInt1TypeInContext(phi_context);
	LLVMTypeRef lhstype = LLVMTypeOf(lhs);
	LLVMTypeRef rhstype = LLVMTypeOf(rhs);
	LLVMTypeKind lhskind = LLVMGetTypeKind(lhstype);
	LLVMTypeKind rhskind = LLVMGetTypeKind(rhstype);

	if (lhskind != rhskind)
		return logError("No addition available for variables of different type.", 0x2521);

	if (lhstype == booltype)
	{
		if (rhstype != booltype)
			return logError("No way to add Boolean to other type.", 0x2522);
		return LLVMBuildXor(phi_builder, lhs, rhs, "xortmp");
	}
	else if (lhskind == LLVMDoubleTypeKind)
		return LLVMBuildFAdd(phi_builder, lhs, rhs, "addtmp");
	else if (lhskind == LLVMIntegerTypeKind)
		return LLVMBuildAdd(phi_builder, lhs, rhs, "addtmp");

	return logError("Incompatible Types for binary '+'.", 0x2523);
}

LLVMValueRef buildAppropriateMultiplication (LLVMValueRef lhs, LLVMValueRef rhs)
{
	LLVMTypeRef booltype = LLVMInt1TypeInContext(phi_context);
	LLVMTypeRef lhstype = LLVMTypeOf(lhs);
	LLVMTypeRef rhstype = LLVMTypeOf(rhs);
	LLVMTypeKind lhskind = LLVMGetTypeKind(lhstype);
	LLVMTypeKind rhskind = LLVMGetTypeKind(rhstype);

	if (lhstype == booltype)
	{
		if (rhstype == booltype)
			return LLVMBuildAnd(phi_builder, lhs, rhs, "andtmp");
		LLVMValueRef null = LLVMConstNull(rhstype);
		return LLVMBuildSelect(phi_builder, lhs, rhs, null, "boolSelect");
	}
	else if (rhstype == booltype)
	{
		LLVMValueRef null = LLVMConstNull(lhstype);
		return LLVMBuildSelect(phi_builder, rhs, lhs, null, "boolselect");
	}
	else if (lhskind != rhskind)
		return logError("No Multiplication for variables of differing type.", 0x2511);
	else if (lhskind == LLVMDoubleTypeKind)
		return LLVMBuildFMul(phi_builder, lhs, rhs, "fmultmp");
	else if (lhskind == LLVMIntegerTypeKind)
		return LLVMBuildMul(phi_builder, lhs, rhs, "imultmp");
	return logError("Incompatible Types for binary '*'.", 0x2512);
}

LLVMValueRef buildAppropriateCmp (LLVMValueRef lhs, LLVMValueRef rhs)
{
	LLVMTypeRef doubletype = LLVMDoubleTypeInContext(phi_context);
	LLVMTypeRef booltype = LLVMInt1TypeInContext(phi_context);
	LLVMTypeRef lhstype = LLVMTypeOf(lhs);
	LLVMTypeRef rhstype = LLVMTypeOf(rhs);

	if (lhstype == booltype)
	{
		if (rhstype != booltype)
			return logError("Cannot compare Boolean and Real.", 0x2531);
		LLVMValueRef notlhs = LLVMBuildNot(phi_builder, lhs, "notlhs");
		return LLVMBuildAnd(phi_builder, notlhs, rhs, "boolcmp");
	}
	else if (lhstype == doubletype)
	{
		if (rhstype != doubletype)
			return logError("Cannot compare Variables of different type.", 0x2532);
		return LLVMBuildFCmp(phi_builder, LLVMRealOLT, lhs, rhs, "lttmp");
	}
	return logError("Incompatible types for binary '<'.", 0x2533);
}

LLVMValueRef buildAppropriateEq (LLVMValueRef lhs, LLVMValueRef rhs)
{
	LLVMTypeKind lhskind = LLVMGetTypeKind(LLVMTypeOf(lhs));
	LLVMTypeKind rhskind = LLVMGetTypeKind(LLVMTypeOf(rhs));

	if (lhskind == LLVMDoubleTypeKind)
	{
		if (rhskind != LLVMDoubleTypeKind)
			return logError("Cannot compare Values of type Real and Bool.", 0x2541);
		return LLVMBuildFCmp(phi_builder, LLVMRealOEQ, lhs, rhs, "eqtmp");
	}
	else if (lhskind == LLVMIntegerTypeKind)
	{
		if (rhskind != LLVMIntegerTypeKind)
			return logError("Cannot compare values of type Integer and Bool.", 0x2542);
		return LLVMBuildICmp(phi_builder, LLVMIntEQ, lhs, rhs, "eqtmp");
	}
	return logError("Incompatible types for binary '='.", 0x2543);
}
