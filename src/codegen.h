#ifndef CODEGEN_H_
#define CODEGEN_H_

#include "ast.h"
#include <llvm-c/Types.h>

LLVMValueRef codegen (Expr *e, int newScope);
#endif /* CODEGEN_H_ */
