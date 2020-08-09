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

	/* For Vector Types, check the element types */
	if (lhskind == LLVMVectorTypeKind)
	{
		if (rhskind != LLVMVectorTypeKind)
			return logError("No addition available for vector and scalar.", 0x2511);
		unsigned lhssize = LLVMGetVectorSize(lhstype);
		unsigned rhssize = LLVMGetVectorSize(rhstype);
		if (rhssize != lhssize)
			return logError("No addition available for vectors of different size.", 0x2513);
		lhstype = LLVMGetElementType(lhstype);
		lhskind = LLVMGetTypeKind(lhstype);
		rhstype = LLVMGetElementType(rhstype);
		rhskind = LLVMGetTypeKind(rhstype);
	}
	else if (rhskind == LLVMVectorTypeKind)
		/* This part executes, if rhs is a vector, but lhs isn't. */
		return logError("No addition available for scalar and vector.", 0x2512);

	if (lhstype == booltype)
	{
		if (rhstype != booltype)
			return logError("No addition available for Boolean and other type.", 0x2514);
		return LLVMBuildXor(phi_builder, lhs, rhs, "xortmp");
	}
	else if (lhskind == LLVMDoubleTypeKind)
	{
		if (rhskind == LLVMIntegerTypeKind)
			/* We need cannot use lhstype, because it was overwritten! */
			rhs = LLVMBuildSIToFP(phi_builder, rhs, LLVMTypeOf(lhs), "sitofp");
		return LLVMBuildFAdd(phi_builder, lhs, rhs, "faddtmp");
	}
	else if (lhskind == LLVMIntegerTypeKind)
	{
		if (rhskind == LLVMDoubleTypeKind)
		{
			lhs = LLVMBuildSIToFP(phi_builder, lhs, LLVMTypeOf(rhs), "sitofp");
			return LLVMBuildFAdd(phi_builder, lhs, rhs, "faddtmp");
		}
		return LLVMBuildAdd(phi_builder, lhs, rhs, "faddtmp");
	}

	return logError("Incompatible Types for binary '+'.", 0x2515);
}

LLVMValueRef buildAppropriateSubtraction (LLVMValueRef lhs, LLVMValueRef rhs)
{
	LLVMTypeRef booltype = LLVMInt1TypeInContext(phi_context);
	LLVMTypeRef lhstype = LLVMTypeOf(lhs);
	LLVMTypeRef rhstype = LLVMTypeOf(rhs);
	LLVMTypeKind lhskind = LLVMGetTypeKind(lhstype);
	LLVMTypeKind rhskind = LLVMGetTypeKind(rhstype);

	/* For Vector Types, check the element types */
	if (lhskind == LLVMVectorTypeKind)
	{
		if (rhskind != LLVMVectorTypeKind)
			return logError("No subtraction available for vector and scalar.", 0x2521);
		unsigned lhssize = LLVMGetVectorSize(lhstype);
		unsigned rhssize = LLVMGetVectorSize(rhstype);
		if (rhssize != lhssize)
			return logError("No subtraction available for vectors of different size.", 0x2523);
		lhstype = LLVMGetElementType(lhstype);
		lhskind = LLVMGetTypeKind(lhstype);
		rhstype = LLVMGetElementType(rhstype);
		rhskind = LLVMGetTypeKind(rhstype);
	}
	else if (rhskind == LLVMVectorTypeKind)
		/* This part executes, if rhs is a vector, but lhs isn't. */
		return logError("No subtraction available for scalar and vector.", 0x2522);

	if (lhstype == booltype)
		return logError("No subtraction available for Boolean and other type.", 0x2524);
	else if (rhstype == booltype)
		return logError("No subtraction available for other type and Boolean.", 0x2525);
	else if (lhskind == LLVMDoubleTypeKind)
	{
		if (rhskind == LLVMIntegerTypeKind)
			rhs = LLVMBuildSIToFP(phi_builder, rhs, LLVMTypeOf(lhs), "sitofp");
		return LLVMBuildFSub(phi_builder, lhs, rhs, "fsubtmp");
	}
	else if (lhskind == LLVMIntegerTypeKind)
	{
		if (rhskind == LLVMDoubleTypeKind)
		{
			lhs = LLVMBuildSIToFP(phi_builder, lhs, LLVMTypeOf(rhs), "sitofp");
			return LLVMBuildFSub(phi_builder, lhs, rhs, "fsubtmp");
		}
		return LLVMBuildSub(phi_builder, lhs, rhs, "ssubtmp");
	}
	return logError("Incompatible types for binary '-'.", 0x2526);
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
				return logError("No multiplication available for vectors of different size.", 0x2531);
			lhstype = LLVMGetElementType(lhstype);
			lhskind = LLVMGetTypeKind(lhstype);
			rhstype = LLVMGetElementType(rhstype);
			rhskind = LLVMGetTypeKind(rhstype);
		}
		else if (rhstype != booltype)
			return logError("No multiplication available for vector and scalar.", 0x2532);
	}

	if (lhstype == booltype)
	{
		if (rhstype == booltype)
			return LLVMBuildAnd(phi_builder, lhs, rhs, "andtmp");
		else if (lhsIsVec)
			return logError("Using Boolean Vectors as Conditional is currently unsupported.", 0x2533);
		LLVMValueRef null = LLVMConstNull(rhstype);
		return LLVMBuildSelect(phi_builder, lhs, rhs, null, "boolSelect");
	}
	else if (rhstype == booltype)
	{
		if (rhsIsVec)
			return logError("Using Boolean Vectors as Conditional is currently unsupported.", 0x2534);
		LLVMValueRef null = LLVMConstNull(lhstype);
		return LLVMBuildSelect(phi_builder, rhs, lhs, null, "boolselect");
	}
	else if (lhskind == LLVMDoubleTypeKind)
	{
		if (rhskind == LLVMIntegerTypeKind)
			rhs = LLVMBuildSIToFP(phi_builder, rhs, LLVMTypeOf(lhs), "sitofp");
		return LLVMBuildFMul(phi_builder, lhs, rhs, "fmultmp");
	}
	else if (lhskind == LLVMIntegerTypeKind)
	{
		if (rhskind == LLVMDoubleTypeKind)
		{
			lhs = LLVMBuildSIToFP(phi_builder, lhs, LLVMTypeOf(rhs), "sitofp");
			return LLVMBuildFMul(phi_builder, lhs, rhs, "fmultmp");
		}
		return LLVMBuildMul(phi_builder, lhs, rhs, "smultmp");
	}
	return logError("Incompatible Types for binary '*'.", 0x2536);
}

LLVMValueRef buildAppropriateDivision (LLVMValueRef lhs, LLVMValueRef rhs)
{
	LLVMTypeRef lhstype = LLVMTypeOf(lhs);
	LLVMTypeRef rhstype = LLVMTypeOf(rhs);
	LLVMTypeKind lhskind = LLVMGetTypeKind(lhstype);
	LLVMTypeKind rhskind = LLVMGetTypeKind(rhstype);

	LLVMTypeRef booltype = LLVMInt1TypeInContext(phi_context);
	if (lhstype == booltype)
		return logError("Cannot divide Boolean.", 0x2541);
	if (lhstype == booltype)
		return logError("Cannot divide by Boolean.", 0x2542);

	/* For Vector Types, check the element types */
	if (lhskind == LLVMVectorTypeKind)
	{
		if (rhskind != LLVMVectorTypeKind)
			return logError("No division available for vector and scalar.", 0x2543);
		unsigned lhssize = LLVMGetVectorSize(lhstype);
		unsigned rhssize = LLVMGetVectorSize(rhstype);
		if (rhssize != lhssize)
			return logError("No addition available for vectors of different size.", 0x2545);
		lhstype = LLVMGetElementType(lhstype);
		lhskind = LLVMGetTypeKind(lhstype);
		rhstype = LLVMGetElementType(rhstype);
		rhskind = LLVMGetTypeKind(rhstype);
	}
	else if (rhskind == LLVMVectorTypeKind)
		/* This part executes if rhs is a vector, but lhs is not. */
		return logError("No division available for scalar and vector.", 0x2544);

	if (lhskind == LLVMDoubleTypeKind)
	{
		if (rhskind == LLVMIntegerTypeKind)
			rhs = LLVMBuildSIToFP(phi_builder, rhs, LLVMTypeOf(lhs), "sitofp");
		return LLVMBuildFDiv(phi_builder, lhs, rhs, "fdivtmp");
	}
	else if (lhskind == LLVMIntegerTypeKind)
	{
		if (rhskind == LLVMDoubleTypeKind)
		{
			lhs = LLVMBuildSIToFP(phi_builder, lhs, LLVMTypeOf(rhs), "sitofp");
			return LLVMBuildFDiv(phi_builder, lhs, rhs, "fdivtmp");
		}
		return LLVMBuildSDiv(phi_builder, lhs, rhs, "sdivtmp");
	}
	return logError("Incompatible types for binary '/'.", 0x2545);
}

LLVMValueRef buildAppropriateComparison (LLVMValueRef lhs, LLVMValueRef rhs)
{
	LLVMTypeRef booltype = LLVMInt1TypeInContext(phi_context);
	LLVMTypeRef lhstype = LLVMTypeOf(lhs);
	LLVMTypeRef rhstype = LLVMTypeOf(rhs);
	LLVMTypeKind lhskind = LLVMGetTypeKind(lhstype);
	LLVMTypeKind rhskind = LLVMGetTypeKind(rhstype);

	if (lhstype == booltype)
	{
		if (rhstype != booltype)
			return logError("Cannot compare Boolean and other type.", 0x2551);
		LLVMValueRef notlhs = LLVMBuildNot(phi_builder, lhs, "notlhs");
		return LLVMBuildAnd(phi_builder, notlhs, rhs, "boolcmp");
	}
	else if (rhstype == booltype)
		return logError("Cannot compare other type and Boolean.", 0x2552);
	else if (lhskind == LLVMDoubleTypeKind)
	{
		if (rhskind == LLVMIntegerTypeKind)
			rhs = LLVMBuildSIToFP(phi_builder, rhs, LLVMTypeOf(lhs), "sitofp");
		return LLVMBuildFCmp(phi_builder, LLVMRealOLT, lhs, rhs, "lttmp");
	}
	else if (lhskind == LLVMIntegerTypeKind)
	{
		if (rhskind == LLVMDoubleTypeKind)
		{
			lhs = LLVMBuildSIToFP(phi_builder, lhs, LLVMTypeOf(rhs), "sitofp");
			return LLVMBuildFCmp(phi_builder, LLVMRealOLT, lhs, rhs, "lttmp");
		}
		return LLVMBuildICmp(phi_builder, LLVMIntSLT, lhs, rhs, "lttmp");
	}
	return logError("Incompatible types for binary '<'.", 0x2553);
}

LLVMValueRef buildAppropriateEquality (LLVMValueRef lhs, LLVMValueRef rhs)
{
	LLVMTypeRef lhstype = LLVMTypeOf(lhs);
	LLVMTypeRef rhstype = LLVMTypeOf(rhs);
	LLVMTypeKind lhskind = LLVMGetTypeKind(lhstype);
	LLVMTypeKind rhskind = LLVMGetTypeKind(rhstype);

	if (lhskind == LLVMVectorTypeKind)
	{
		if (rhskind != LLVMVectorTypeKind)
			return logError("No Equality available between scalar and Vector.", 0x2561);
		if (LLVMGetVectorSize(lhstype) != LLVMGetVectorSize(rhstype))
			return logError("No Equality available between vectors of different size.", 0x2563);
		lhstype = LLVMGetElementType(lhstype);
		lhskind = LLVMGetTypeKind(lhstype);
		rhstype = LLVMGetElementType(rhstype);
		rhskind = LLVMGetTypeKind(rhstype);
	}
	else if (rhskind == LLVMVectorTypeKind)
		return logError("No Equality available between vector and scalar.", 0x2562);

	if (lhskind == LLVMDoubleTypeKind)
	{
		if (rhskind == LLVMIntegerTypeKind)
			rhs = LLVMBuildSIToFP(phi_builder, rhs, LLVMTypeOf(lhs), "sitofp");
		return LLVMBuildFCmp(phi_builder, LLVMRealOEQ, lhs, rhs, "feqtmp");
	}
	else if (lhskind == LLVMIntegerTypeKind)
	{
		if (rhskind == LLVMDoubleTypeKind)
		{
			lhs = LLVMBuildSIToFP(phi_builder, lhs, LLVMTypeOf(rhs), "sitofp");
			return LLVMBuildFCmp(phi_builder, LLVMRealOEQ, lhs, rhs, "feqtmp");
		}
		return LLVMBuildICmp(phi_builder, LLVMIntEQ, lhs, rhs, "seqtmp");
	}
	return logError("Incompatible types for binary '='.", 0x2564);
}

LLVMValueRef buildAppropriateModulo (LLVMValueRef lhs, LLVMValueRef rhs)
{
	LLVMTypeRef booltype = LLVMInt1TypeInContext(phi_context);
	LLVMTypeRef lhstype = LLVMTypeOf(lhs);
	LLVMTypeRef rhstype = LLVMTypeOf(rhs);
	LLVMTypeKind lhskind = LLVMGetTypeKind(lhstype);
	LLVMTypeKind rhskind = LLVMGetTypeKind(rhstype);

	LLVMValueRef zero = LLVMConstNull(rhstype);
	LLVMValueRef naive;

	/* Booleans are not supported for Modulo */
	if (lhstype == booltype)
		return logError("No Modulo available for Boolean and other type.", 0x2571);
	if (rhstype == booltype)
		return logError("No Modulo available for other type and Boolean.", 0x2572);

	/* For Vector Types, check the element types */
	if (lhskind == LLVMVectorTypeKind)
	{
		if (rhskind != LLVMVectorTypeKind)
			return logError("No Modulo available for vector and scalar.", 0x2573);
		unsigned lhssize = LLVMGetVectorSize(lhstype);
		unsigned rhssize = LLVMGetVectorSize(rhstype);
		if (rhssize != lhssize)
			return logError("No modulo available for vectors of different size.", 0x2575);
		lhstype = LLVMGetElementType(lhstype);
		lhskind = LLVMGetTypeKind(lhstype);
		rhstype = LLVMGetElementType(rhstype);
		rhskind = LLVMGetTypeKind(rhstype);
	}
	else if (rhskind == LLVMVectorTypeKind)
		return logError("No modulo available for scalar and vector.", 0x2574);

	if (lhskind == LLVMDoubleTypeKind)
	{
		if (rhskind == LLVMIntegerTypeKind)
			rhs = LLVMBuildSIToFP(phi_builder, rhs, LLVMTypeOf(lhs), "sitofp");
		naive = LLVMBuildFRem(phi_builder, lhs, rhs, "naiveFRem");
	}
	else if (lhskind == LLVMIntegerTypeKind)
	{
		if (rhskind == LLVMDoubleTypeKind)
		{
			lhs = LLVMBuildSIToFP(phi_builder, lhs, LLVMTypeOf(rhs), "sitofp");
			naive = LLVMBuildFRem(phi_builder, lhs, rhs, "naiveFRem");
		}
		else
			naive = LLVMBuildSRem(phi_builder, lhs, rhs, "naiveSRem");
	}
	else
		return logError("Incompatible types for binary '%'.", 0x2576);

	LLVMValueRef isZero = buildAppropriateEquality(rhs, zero);
	return LLVMBuildSelect(phi_builder, isZero, lhs, naive, "remSelect");
}
