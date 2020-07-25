#ifndef PARSER_H_
#define PARSER_H_

typedef struct NumExprAST {
	double val;
} NumExpr;

typedef struct VarExprAST {
	char *name;
} VarExpr;

typedef struct BinaryExprAST {
	int op;
	void *LHS, *RHS;
} BinaryExpr;

typedef struct CallExprAST {
	char *callee;
	void **args;
} CallExpr;

void* parseNumberExpr (token *curtok);
void* parseParenExpr (token *curtok);
void* parseBinOpRHS (token *curtok, int minPrec, void* LHS);

void* parseExpression (token *curtok);
void* parsePrimary (token *curtok);

#endif /* PARSER_H_ */
