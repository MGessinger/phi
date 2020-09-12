#ifndef CODEGEN_H_
#define CODEGEN_H_

#include "ast.h"
#include <llvm-c/Types.h>

LLVMTypeRef getAppropriateType (int typename);
LLVMValueRef codegen (Expr *e, int newScope);
#endif /* CODEGEN_H_ */
