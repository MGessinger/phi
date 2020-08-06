#ifndef BINARYOPS_H_
#define BINARYOPS_H_

LLVMValueRef buildAppropriateAddition (LLVMValueRef lhs, LLVMValueRef rhs);
LLVMValueRef buildAppropriateMultiplication (LLVMValueRef lhs, LLVMValueRef rhs);
LLVMValueRef buildAppropriateDivision (LLVMValueRef lhs, LLVMValueRef rhs);
LLVMValueRef buildAppropriateCmp (LLVMValueRef lhs, LLVMValueRef rhs);
LLVMValueRef buildAppropriateEq (LLVMValueRef lhs, LLVMValueRef rhs);
LLVMValueRef buildAppropriateMod (LLVMValueRef lhs, LLVMValueRef rhs);

#endif /* BINARYOPS_H_ */
