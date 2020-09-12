#ifndef TEMPLATING_H_
#define TEMPLATING_H_

#include <llvm-c/Core.h>
#include "ast.h"

void clearTemplates();
void defineNewTemplate (FunctionExpr *ie);
void declareNewTemplate (ProtoExpr *pe);
LLVMValueRef tryGetTemplate (const char *name, int type_name);
LLVMValueRef compileTemplatePredefined (Expr *e, int type_name);

#endif /* TEMPLATING_H_ */
