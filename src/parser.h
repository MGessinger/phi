#ifndef PARSER_H_
#define PARSER_H_

#include "ast.h"
#include "lexer.h"

void* logError (const char *errMsg, int errCode);

Expr* parseNumberExpr ();
Expr* parseParenExpr ();
Expr* parseBinOpRHS (int minPrec, Expr* LHS);

Expr* parsePrototype ();
Expr* parseDefinition ();
Expr* parseExtern ();
Expr* parseTopLevelExpr ();

Expr* parseExpression ();
Expr* parsePrimary ();

extern token *curtok;

#endif /* PARSER_H_ */
