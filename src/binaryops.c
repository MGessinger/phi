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

	/* Assert equal typekinds (no automatic type casting is performed here) */
	if (lhskind != rhskind)
		return logError("No addition available for variables of different type.", 0x2511);

	/* For Vector Types, check the element types */
	if (lhskind == LLVMVectorTypeKind)
	{
		unsigned lhssize = LLVMGetVectorSize(lhstype);
		unsigned rhssize = LLVMGetVectorSize(rhstype);
		if (rhssize != lhssize)
			return logError("No addition available for vectors of different size.", 0x2512);
		lhstype = LLVMGetElementType(lhstype);
		lhskind = LLVMGetTypeKind(lhstype);
		rhstype = LLVMGetElementType(rhstype);
		rhskind = LLVMGetTypeKind(rhstype);

		if (lhskind != rhskind)
			return logError("No addition available for vectors of different type.", 0x2513);
	}

	if (lhstype == booltype)
	{
		if (rhstype != booltype)
			return logError("No way to add Boolean to other type.", 0x2514);
		return LLVMBuildXor(phi_builder, lhs, rhs, "xortmp");
	}
	else if (lhskind == LLVMDoubleTypeKind)
		return LLVMBuildFAdd(phi_builder, lhs, rhs, "addtmp");
	else if (lhskind == LLVMIntegerTypeKind)
		return LLVMBuildAdd(phi_builder, lhs, rhs, "addtmp");

	return logError("Incompatible Types for binary '+'.", 0x2515);
}

LLVMValueRef buildAppropriateMultiplication (LLVMValueRef lhs, LLVMValueRef rhs)
{
	LLVMTypeRef booltype = LLVMInt1TypeInContext(phi_context);
	LLVMTypeRef lhstype = LLVMTypeOf(lhs);
	LLVMTypeRef rhstype = LLVMTypeOf(rhs);
	LLVMTypeKind lhskind = LLVMGetTypeKind(lhstype);
	LLVMTypeKind rhskind = LLVMGetTypeKind(rhstype);

	int lhsIsVec = (lhskind == LLVMVectorTypeKind);
	int rhsIsVec = (rhskind == LLVMVectorTypeKind);

	if (lhsIsVec)
	{
		if (rhsIsVec)
		{
			if (LLVMGetVectorSize(lhstype) != LLVMGetVectorSize(rhstype))
				return logError("No multiplication available for vectors of different size.", 0x2521);
			lhstype = LLVMGetElementType(lhstype);
			lhskind = LLVMGetTypeKind(lhstype);
			rhstype = LLVMGetElementType(rhstype);
			rhskind = LLVMGetTypeKind(rhstype);
		}
		else if (rhstype != booltype)
			return logError("No multiplication available for vector and scalar.", 0x2522);
	}

	if (lhstype == booltype)
	{
		if (rhstype == booltype)
			return LLVMBuildAnd(phi_builder, lhs, rhs, "andtmp");
		else if (lhsIsVec)
			return logError("Using Boolean Vectors as Conditional is currently unsupported.", 0x2523);
		LLVMValueRef null = LLVMConstNull(rhstype);
		return LLVMBuildSelect(phi_builder, lhs, rhs, null, "boolSelect");
	}
	else if (rhstype == booltype)
	{
		if (rhsIsVec)
			return logError("Using Boolean Vectors as Conditional is currently unsupported.", 0x2524);
		LLVMValueRef null = LLVMConstNull(lhstype);
		return LLVMBuildSelect(phi_builder, rhs, lhs, null, "boolselect");
	}
	else if (lhskind != rhskind)
		return logError("No Multiplication for variables of differing type.", 0x2525);
	else if (lhskind == LLVMDoubleTypeKind)
		return LLVMBuildFMul(phi_builder, lhs, rhs, "fmultmp");
	else if (lhskind == LLVMIntegerTypeKind)
		return LLVMBuildMul(phi_builder, lhs, rhs, "imultmp");
	return logError("Incompatible Types for binary '*'.", 0x2526);
}

LLVMValueRef buildAppropriateDivision (LLVMValueRef lhs, LLVMValueRef rhs)
{
	LLVMTypeRef lhstype = LLVMTypeOf(lhs);
	LLVMTypeRef rhstype = LLVMTypeOf(rhs);
	LLVMTypeKind lhskind = LLVMGetTypeKind(lhstype);
	LLVMTypeKind rhskind = LLVMGetTypeKind(rhstype);

	if (lhskind != rhskind)
		return logError("No Division available for variables of different type.", 0x2531);
	
	/* For Vector Types, check the element types */
	if (lhskind == LLVMVectorTypeKind)
	{
		unsigned lhssize = LLVMGetVectorSize(lhstype);
		unsigned rhssize = LLVMGetVectorSize(rhstype);
		if (rhssize != lhssize)
			return logError("No addition available for vectors of different size.", 0x2532);
		lhstype = LLVMGetElementType(lhstype);
		lhskind = LLVMGetTypeKind(lhstype);
		rhstype = LLVMGetElementType(rhstype);
		rhskind = LLVMGetTypeKind(rhstype);

		if (lhskind != rhskind)
			return logError("No addition available for vectors of different type.", 0x2533);
	}

	if (lhskind == LLVMDoubleTypeKind)
		return LLVMBuildFDiv(phi_builder, lhs, rhs, "fdivtmp");
	else if (lhskind == LLVMIntegerTypeKind)
		return LLVMBuildSDiv(phi_builder, lhs, rhs, "sdivtmp");
	return logError("Incompatible types for binary '/'.", 0x2534);
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
			return logError("Cannot compare Boolean and Real.", 0x2541);
		LLVMValueRef notlhs = LLVMBuildNot(phi_builder, lhs, "notlhs");
		return LLVMBuildAnd(phi_builder, notlhs, rhs, "boolcmp");
	}
	else if (lhstype == doubletype)
	{
		if (rhstype != doubletype)
			return logError("Cannot compare Variables of different type.", 0x2542);
		return LLVMBuildFCmp(phi_builder, LLVMRealOLT, lhs, rhs, "lttmp");
	}
	return logError("Incompatible types for binary '<'.", 0x2543);
}

LLVMValueRef buildAppropriateEq (LLVMValueRef lhs, LLVMValueRef rhs)
{
	LLVMTypeRef lhstype = LLVMTypeOf(lhs);
	LLVMTypeRef rhstype = LLVMTypeOf(rhs);
	LLVMTypeKind lhskind = LLVMGetTypeKind(lhstype);
	LLVMTypeKind rhskind = LLVMGetTypeKind(rhstype);

	int lhsIsVec = (lhskind == LLVMVectorTypeKind);
	int rhsIsVec = (rhskind == LLVMVectorTypeKind);
	if (lhsIsVec ^ rhsIsVec)
		return logError("No Equality available between scalar and Vector.", 0x2551);
	else if (lhsIsVec && rhsIsVec)
	{
		if (LLVMGetVectorSize(lhstype) != LLVMGetVectorSize(rhstype))
			return logError("No Equality available between vectors of different size.", 0x2552);
		lhstype = LLVMGetElementType(lhstype);
		lhskind = LLVMGetTypeKind(lhstype);
		rhstype = LLVMGetElementType(rhstype);
		rhskind = LLVMGetTypeKind(rhstype);
	}

	if (lhskind == LLVMDoubleTypeKind)
	{
		if (rhskind != LLVMDoubleTypeKind)
			return logError("Cannot compare Values of type Real and Bool.", 0x2553);
		return LLVMBuildFCmp(phi_builder, LLVMRealOEQ, lhs, rhs, "eqtmp");
	}
	else if (lhskind == LLVMIntegerTypeKind)
	{
		if (rhskind != LLVMIntegerTypeKind)
			return logError("Cannot compare values of type Integer and Bool.", 0x2554);
		return LLVMBuildICmp(phi_builder, LLVMIntEQ, lhs, rhs, "eqtmp");
	}
	return logError("Incompatible types for binary '='.", 0x2555);
}

LLVMValueRef buildAppropriateMod (LLVMValueRef lhs, LLVMValueRef rhs)
{
	LLVMTypeRef lhstype = LLVMTypeOf(lhs);
	LLVMTypeRef rhstype = LLVMTypeOf(rhs);
	LLVMTypeKind lhskind = LLVMGetTypeKind(lhstype);
	LLVMTypeKind rhskind = LLVMGetTypeKind(rhstype);

	LLVMValueRef zero = LLVMConstNull(rhstype);
	LLVMValueRef naive;

	if (lhskind != rhskind)
		return logError("No modulo available for variables of different type.", 0x2561);

	/* For Vector Types, check the element types */
	if (lhskind == LLVMVectorTypeKind)
	{
		unsigned lhssize = LLVMGetVectorSize(lhstype);
		unsigned rhssize = LLVMGetVectorSize(rhstype);
		if (rhssize != lhssize)
			return logError("No addition available for vectors of different size.", 0x2562);
		lhstype = LLVMGetElementType(lhstype);
		lhskind = LLVMGetTypeKind(lhstype);
		rhstype = LLVMGetElementType(rhstype);
		rhskind = LLVMGetTypeKind(rhstype);

		if (lhskind != rhskind)
			return logError("No addition available for vectors of different type.", 0x2563);
	}

	if (lhskind == LLVMDoubleTypeKind)
		naive = LLVMBuildFRem(phi_builder, lhs, rhs, "naiveFRem");
	else if (lhskind == LLVMIntegerTypeKind)
		naive = LLVMBuildSRem(phi_builder, lhs, rhs, "naiveSRem");
	else
		return logError("Incompatible types for binary '%'.", 0x2564);
	LLVMValueRef isZero = buildAppropriateEq(rhs, zero);

	return LLVMBuildSelect(phi_builder, isZero, lhs, naive, "remSelect");
}
