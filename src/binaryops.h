#ifndef BINARYOPS_H_
#define BINARYOPS_H_

LLVMValueRef buildAppropriateAddition (LLVMValueRef lhs, LLVMValueRef rhs);
LLVMValueRef buildAppropriateSubtraction (LLVMValueRef lhs, LLVMValueRef rhs);
LLVMValueRef buildAppropriateMultiplication (LLVMValueRef lhs, LLVMValueRef rhs);
LLVMValueRef buildAppropriateDivision (LLVMValueRef lhs, LLVMValueRef rhs);
LLVMValueRef buildAppropriateComparison (LLVMValueRef lhs, LLVMValueRef rhs);
LLVMValueRef buildAppropriateEquality (LLVMValueRef lhs, LLVMValueRef rhs);
LLVMValueRef buildAppropriateModulo (LLVMValueRef lhs, LLVMValueRef rhs);

#endif /* BINARYOPS_H_ */
