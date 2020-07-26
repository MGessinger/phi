#ifndef CODEGEN_H_
#define CODEGEN_H_

#include "ast.h"
#include <llvm-c/Types.h>

void initialiseLLVM ();
void shutdownLLVM ();

LLVMValueRef codegen (Expr *e);
#endif /* CODEGEN_H_ */
